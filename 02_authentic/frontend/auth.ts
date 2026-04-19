import NextAuth from "next-auth";
import Keycloak from "next-auth/providers/keycloak";
import type { JWT } from "next-auth/jwt";

/**
 * Публичный issuer (URL в браузере) и внутренний base URL для server-side fetch из Docker.
 * В контейнере frontend `localhost:8080` недоступен — token/userinfo идут на hostname `keycloak`.
 * Authorization URL остаётся на localhost, чтобы редирект открывался у пользователя.
 * См. docker-compose: AUTH_KEYCLOAK_ISSUER + AUTH_KEYCLOAK_ISSUER_INTERNAL, KC_HOSTNAME на Keycloak.
 */
const issuerPublic =
  process.env.AUTH_KEYCLOAK_ISSUER ?? "http://localhost:8080/realms/demo";
const issuerInternal =
  process.env.AUTH_KEYCLOAK_ISSUER_INTERNAL ?? issuerPublic;

/** Строковый URL: Auth.js делает `new URL(e.url)` — после бандла экземпляр `URL` может ломаться. */
function realmEndpoint(path: string, base: string): string {
  const b = base.replace(/\/$/, "");
  return `${b}/${path}`;
}

export const { handlers, auth, signIn, signOut } = NextAuth({
  trustHost: true,
  providers: [
    Keycloak({
      clientId: process.env.AUTH_KEYCLOAK_ID ?? "demo-frontend",
      clientSecret: process.env.AUTH_KEYCLOAK_SECRET ?? "",
      issuer: issuerPublic,
      authorization: {
        url: realmEndpoint("protocol/openid-connect/auth", issuerPublic),
      },
      token: {
        url: realmEndpoint("protocol/openid-connect/token", issuerInternal),
      },
      userinfo: {
        url: realmEndpoint("protocol/openid-connect/userinfo", issuerInternal),
      },
      client: {
        token_endpoint_auth_method: "none",
      },
    }),
  ],
  callbacks: {
    async jwt({ token, account }): Promise<JWT> {
      if (account?.access_token) {
        token.accessToken = account.access_token as string;
      }
      return token;
    },
    async session({ session, token }) {
      if (token.accessToken) {
        session.accessToken = token.accessToken as string;
      }
      return session;
    },
  },
});
