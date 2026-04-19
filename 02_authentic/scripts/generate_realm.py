#!/usr/bin/env python3
"""Генерирует keycloak/import/demo-realm.json для импорта при старте Keycloak."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "keycloak" / "import" / "demo-realm.json"


def user(username: str, password: str, roles: list[str]) -> dict:
    return {
        "username": username,
        "enabled": True,
        "emailVerified": True,
        "credentials": [{"type": "password", "value": password, "temporary": False}],
        "realmRoles": roles,
    }


def main() -> None:
    users = [user("admin", "admin", ["admin"])]
    for i in range(1, 11):
        users.append(user(f"user{i}", f"password{i}", ["user"]))
    for i in range(1, 11):
        users.append(user(f"puser{i}", f"password{i}", ["power_user"]))

    realm = {
        "realm": "demo",
        "enabled": True,
        "displayName": "Demo (Authentic)",
        "sslRequired": "none",
        "registrationAllowed": False,
        "loginWithEmailAllowed": True,
        "duplicateEmailsAllowed": False,
        "roles": {
            "realm": [
                {"name": "admin", "description": "Realm administrator"},
                {"name": "user", "description": "Standard user"},
                {"name": "power_user", "description": "Power user"},
            ]
        },
        "defaultRoles": ["user"],
        "clients": [
            {
                "clientId": "demo-frontend",
                "name": "Demo Next.js (public client, PKCE)",
                "description": "SPA — Authorization Code + PKCE",
                "enabled": True,
                "publicClient": True,
                "protocol": "openid-connect",
                "standardFlowEnabled": True,
                "implicitFlowEnabled": False,
                "directAccessGrantsEnabled": False,
                "serviceAccountsEnabled": False,
                "redirectUris": ["http://localhost:3000/*"],
                "webOrigins": ["http://localhost:3000"],
                "attributes": {"pkce.code.challenge.method": "S256"},
            }
        ],
        "users": users,
    }

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(realm, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Wrote {OUT}")


if __name__ == "__main__":
    main()
