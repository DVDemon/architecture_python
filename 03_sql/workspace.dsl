/*
  Архитектура: FastAPI + PostgreSQL (main.py, SQLAlchemy).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "REST + SQL (FastAPI)" "CRUD пользователей в PostgreSQL" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "SQLFAST"
        }

        client = person "Клиент API"

        app = softwareSystem "Сервис пользователей (SQL)" "FastAPI, SQLAlchemy, переменная DATABASE_URL" {
            properties {
                cmdb "SQLFAST"
            }

            api = container "REST API" "Эндпоинты для User" {
                tags "api"
                technology "Python 3, FastAPI, SQLAlchemy"
                url "file:./main.py"
                properties {
                    external_name ext_sqlfast_api
                }

                component "ORM и транзакции" {
                    description "Сессии БД, создание таблиц, CRUD"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "users_rest" "REST /users" {
                    properties {
                        type          api
                        protocol      rest
                    }
                }
            }

            postgres = container "PostgreSQL" "Хранение таблицы users" {
                tags "database" "postgresql"
                technology "PostgreSQL"
                properties {
                    entity "User"
                }
            }

            api -> postgres "SQLAlchemy" "TCP :5432" {
                properties {
                    send   "SQL"
                    return "ORM-объекты"
                }
            }
        }

        client -> app.api "HTTP" "HTTP :8000"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "uvicorn + внешний Postgres (docker или локально)" {
                properties {
                    type "local"
                }

                deploymentNode "api" "Процесс FastAPI" {
                    properties {
                        type "process"
                    }
                    containerInstance app.api
                }

                deploymentNode "postgres" "СУБД" {
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
            title "Создание пользователя в БД"
            client -> app.api "POST /users/"
            app.api -> app.postgres "INSERT"
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
