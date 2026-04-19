"use client";

import { signIn, signOut, useSession } from "next-auth/react";
import { useCallback, useState } from "react";

const backend =
  process.env.NEXT_PUBLIC_BACKEND_URL ?? "http://localhost:8000";

export default function Home() {
  const { data: session, status } = useSession();
  const [apiJson, setApiJson] = useState<string | null>(null);
  const [apiError, setApiError] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);

  const callBackend = useCallback(async () => {
    if (!session?.accessToken) {
      setApiError("Нет access token в сессии");
      setApiJson(null);
      return;
    }
    setLoading(true);
    setApiError(null);
    setApiJson(null);
    try {
      const res = await fetch(`${backend}/me`, {
        headers: {
          Authorization: `Bearer ${session.accessToken}`,
        },
      });
      const text = await res.text();
      if (!res.ok) {
        setApiError(`${res.status}: ${text}`);
        return;
      }
      try {
        setApiJson(JSON.stringify(JSON.parse(text), null, 2));
      } catch {
        setApiJson(text);
      }
    } catch (e) {
      setApiError(e instanceof Error ? e.message : String(e));
    } finally {
      setLoading(false);
    }
  }, [session?.accessToken]);

  return (
    <main>
      <h1>OAuth 2.0 + OpenID Connect</h1>
      <p className="lead">
        Клиент: Next.js (Authorization Code + PKCE). Сервер авторизации:
        Keycloak (realm <code>demo</code>, клиент <code>demo-frontend</code>
        ). API: FastAPI с проверкой JWT.
      </p>

      <div className="card">
        {status === "loading" && <p className="muted">Загрузка сессии…</p>}
        {status === "unauthenticated" && (
          <div className="row">
            <button
              type="button"
              className="primary"
              onClick={() => signIn("keycloak")}
            >
              Войти через Authentic (Keycloak)
            </button>
          </div>
        )}
        {status === "authenticated" && session?.user && (
          <>
            <p>
              <strong>Пользователь (OIDC):</strong>{" "}
              {session.user.name ?? session.user.email ?? "—"}
            </p>
            <p className="muted">
              Access token передаётся в заголовке{" "}
              <code>Authorization: Bearer …</code> при вызове backend.
            </p>
            <div className="row">
              <button type="button" className="primary" onClick={callBackend}>
                {loading ? "Запрос…" : "GET /me на backend"}
              </button>
              <button
                type="button"
                className="secondary"
                onClick={() => signOut()}
              >
                Выйти
              </button>
            </div>
          </>
        )}
      </div>

      {(apiJson || apiError) && (
        <div className="card">
          <strong>Ответ Resource Server</strong>
          {apiError && <p className="err">{apiError}</p>}
          {apiJson && <pre>{apiJson}</pre>}
        </div>
      )}
    </main>
  );
}
