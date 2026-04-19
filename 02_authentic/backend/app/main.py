import os
from typing import Annotated, Any

import jwt
from fastapi import Depends, FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.security import HTTPAuthorizationCredentials, HTTPBearer
from jwt import PyJWKClient

OIDC_ISSUER = os.environ["OIDC_ISSUER"].rstrip("/")
JWKS_URL = os.environ["JWKS_URL"]
EXPECTED_AZP = os.environ.get("OIDC_CLIENT_ID", "demo-frontend")

_jwks = PyJWKClient(JWKS_URL)
_bearer = HTTPBearer(auto_error=False)

app = FastAPI(title="Demo Resource Server", version="0.1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:3000",
        "http://127.0.0.1:3000",
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def decode_access_token(token: str) -> dict[str, Any]:
    try:
        signing_key = _jwks.get_signing_key_from_jwt(token)
        payload = jwt.decode(
            token,
            signing_key.key,
            algorithms=["RS256"],
            issuer=OIDC_ISSUER,
            options={
                "verify_aud": False,
            },
        )
    except jwt.exceptions.PyJWTError as e:
        raise HTTPException(status_code=401, detail=f"Invalid token: {e}") from e

    if payload.get("azp") != EXPECTED_AZP:
        raise HTTPException(status_code=403, detail="Token not issued for this client")

    return payload


async def current_user_payload(
    cred: Annotated[HTTPAuthorizationCredentials | None, Depends(_bearer)],
) -> dict[str, Any]:
    if cred is None or cred.scheme.lower() != "bearer":
        raise HTTPException(status_code=401, detail="Missing Bearer token")
    return decode_access_token(cred.credentials)


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/me")
def me(payload: Annotated[dict[str, Any], Depends(current_user_payload)]) -> dict[str, Any]:
    realm_access = payload.get("realm_access") or {}
    roles = realm_access.get("roles") or []
    return {
        "sub": payload.get("sub"),
        "preferred_username": payload.get("preferred_username"),
        "email": payload.get("email"),
        "realm_roles": roles,
        "iss": payload.get("iss"),
        "azp": payload.get("azp"),
    }
