/*
  Архитектура: Poco REST + PostgreSQL (docker-compose.yaml).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Poco REST + PostgreSQL" "REST API и персистентность в Postgres" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "POCOPG"
        }

        student = person "Клиент API"

        app = softwareSystem "Poco Server с PostgreSQL" "Шаблон: JWT, CRUD, Swagger; данные в PG" {
            properties {
                cmdb "POCOPG"
            }

            api = container "REST API" "Доступ к сущностям через SQL (PGHOST=postgres)" {
                tags "api"
                technology "C++17, POCO, libpq"
                url "file:./src"
                properties {
                    external_name ext_poco_pg_api
                }

                component "Доступ к данным" {
                    description "Репозитории, SQL, миграции схемы через приложение"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "rest_api" "HTTP API" {
                    properties {
                        type          api
                        protocol      rest
                        "REST"        rps=40;latency=150;error_rate=0.01
                    }
                }
            }

            postgres = container "PostgreSQL" "БД poco_template" {
                tags "database" "postgresql"
                technology "PostgreSQL 16"
                properties {
                    entity "Пользователи, организации (учебная модель)"
                }
            }

            api -> postgres "Чтение/запись" "TCP :5432" {
                properties {
                    send   "SQL"
                    return "Строки, результаты запросов"
                }
            }
        }

        student -> app.api "REST-вызовы" "HTTP :8080"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Docker Compose (см. docker-compose.yaml)" {
                properties {
                    type "docker-compose"
                }

                deploymentNode "poco_template_server" "Контейнер API (8080)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance app.api
                }

                deploymentNode "postgres" "Контейнер PostgreSQL (5432)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance app.postgres
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

        systemContext app {
            include *
            autoLayout lr
        }

        container app {
            include *
            autoLayout lr
        }

        component app.api {
            include *
            autoLayout
        }

        dynamic app "001" {
            autoLayout
            title "Запрос с записью в БД"
            student -> app.api "HTTP запрос"
            app.api -> app.postgres "SELECT/INSERT\n:5432"
        }

        deployment app "Development" {
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
