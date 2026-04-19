/*
  Архитектура учебного стенда OAuth 2.0 + OIDC (Keycloak, Next.js, FastAPI).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Демо OAuth 2.0 / OIDC" "Архитектура стенда: Authentic (Keycloak), Frontend, Resource Server" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "AUTHDEMO"
        }

        # [3.1] Внешняя по отношению к приложению роль — браузерный пользователь и админ IdP
        student = person "Пользователь (студент)"
        operator = person "Администратор IdP"

        authdemo = softwareSystem "Демо аутентификации OAuth 2.0" "Веб-клиент (OIDC), Resource Server (JWT), Keycloak как Authentic" {
            properties {
                cmdb "AUTHDEMO"
            }

            frontend = container "Frontend" "SPA: Next.js, Auth.js — Authorization Code + PKCE" {
                tags "web"
                technology "Next.js 14, TypeScript"
                url "file:./frontend"
                properties {
                    external_name ext_authdemo_frontend
                }

                component "Сессия и вызов API" {
                    description "Отображение identity из OIDC и передача access token в Resource Server"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "protected_fetch" "Запросы к backend с Bearer" {
                    properties {
                        type          api
                        protocol      rest
                        "GET /me"     rps=20;latency=300;error_rate=0.01
                    }
                }
            }

            backend = container "Backend (Resource Server)" "REST API: проверка JWT (JWKS), выдача данных о субъекте" {
                tags "api"
                technology "Python 3.12, FastAPI"
                url "file:./backend"
                properties {
                    external_name ext_authdemo_api
                }

                component "Проверка access token" {
                    description "Подпись RS256 по JWKS, iss, exp, azp (клиент demo-frontend)"
                    properties {
                        type        capability
                        code        002
                        version     "1.0"
                    }
                }

                component "me_resource" "GET /me" {
                    properties {
                        type          api
                        api_url       "http://localhost:8000/docs"
                        protocol      rest
                    }
                }
            }

            authentic = container "Authentic (Authorization Server)" "Keycloak: realm demo, пользователи, клиент OIDC, выдача токенов" {
                tags "keycloak"
                technology "Keycloak 26"
                properties {
                    external_name ext_authdemo_keycloak
                }
            }

            keycloak_db = container "PostgreSQL (Keycloak)" "Персистентность пользователей и конфигурации realm" {
                tags "database" "postgresql"
                technology "PostgreSQL 16"
                properties {
                    entity "User, Realm, Client"
                }
            }

            frontend -> authentic "OAuth 2.0 / OIDC: authorize, token (PKCE)" "HTTPS :8080"
            authentic -> frontend "HTTP redirect с authorization code на redirect_uri" "HTTPS :3000"
            frontend -> backend "REST, Authorization: Bearer (access_token)" "HTTP :8000"
            backend -> authentic "OpenID Provider JWKS и issuer (валидация JWT)" "HTTP :8080"
            authentic -> keycloak_db "Данные IdP" "TCP :5432" {
                properties {
                    send   "Учётные записи, realm, клиенты"
                    return "Конфигурация для выдачи токенов"
                }
            }
        }

        student -> authdemo.frontend "Использует UI в браузере" "HTTPS :3000"
        student -> authdemo.authentic "Аутентификация на странице входа IdP" "HTTPS :8080"
        operator -> authdemo.authentic "Администрирует realm и клиентов" "HTTPS :8080"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Docker Compose (см. docker-compose.yml)" {
                properties {
                    type "docker-compose"
                }

                deploymentNode "frontend" "Контейнер Next.js (порт 3000)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance authdemo.frontend
                }

                deploymentNode "backend" "Контейнер FastAPI (порт 8000)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance authdemo.backend
                }

                deploymentNode "keycloak" "Контейнер Keycloak / Authentic (порт 8080)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance authdemo.authentic
                }

                deploymentNode "postgres" "Контейнер PostgreSQL для Keycloak" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance authdemo.keycloak_db
                }
            }
        }
    }

    views {
        properties {
            kroki.url    "https://kroki.io"
            kroki.format "svg"
        }

        themes default

        systemContext authdemo {
            include *
            autoLayout lr
        }

        container authdemo {
            include *
            autoLayout lr
        }

        component authdemo.backend {
            include *
            autoLayout
        }

        component authdemo.frontend {
            include *
            autoLayout
        }

        dynamic authdemo "001" {
            autoLayout
            title "Вход OIDC и вызов защищённого API"
            student -> authdemo.frontend "Открыть приложение (localhost:3000)"
            authdemo.frontend -> authdemo.authentic "GET /auth (OIDC): redirect на IdP"
            student -> authdemo.authentic "Логин и пароль"
            authdemo.authentic -> authdemo.frontend "Redirect на redirect_uri с code"
            authdemo.frontend -> authdemo.authentic "POST token: code + code_verifier → tokens"
            authdemo.frontend -> authdemo.backend "GET /me с Bearer access_token"
            authdemo.backend -> authdemo.authentic "GET JWKS / проверка iss"
        }

        deployment authdemo "Development" {
            include *
            autoLayout
        }

        styles {
            element "Database" {
                shape Cylinder
            }
        }
    }
}
