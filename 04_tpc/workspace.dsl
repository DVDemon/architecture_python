/*
  Архитектура: демо двухфазного коммита (2PC) на FastAPI в памяти.
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "TPC / 2PC (FastAPI)" "Пользователи и транзакции: create / commit / abort" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "TPCDEMO"
        }

        client = person "Клиент API"

        tpc = softwareSystem "Демо транзакций (2PC)" "FastAPI, in-memory users_db и transactions" {
            properties {
                cmdb "TPCDEMO"
            }

            api = container "Transaction API" "Эндпоинты /users, операции со статусом транзакции" {
                tags "api"
                technology "Python 3, FastAPI"
                url "file:./main.py"
                properties {
                    external_name ext_tpc_api
                }

                component "Управление 2PC" {
                    description "Создание транзакции, commit, abort; применение к users_db"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "users_api" "REST пользователей и транзакций" {
                    properties {
                        type          api
                        protocol      rest
                    }
                }
            }
        }

        client -> tpc.api "HTTP вызовы" "HTTP :8000"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "uvicorn main:app" {
                properties {
                    type "local"
                }

                deploymentNode "api" "Процесс FastAPI (порт 8000)" {
                    properties {
                        type "process"
                    }
                    containerInstance tpc.api
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

        systemContext tpc {
            include *
            autoLayout lr
        }

        container tpc {
            include *
            autoLayout lr
        }

        component tpc.api {
            include *
            autoLayout
        }

        dynamic tpc "001" {
            autoLayout
            title "Создание пользователя в транзакции"
            client -> tpc.api "POST /users"
            client -> tpc.api "POST .../commit"
        }

        deployment tpc "Development" {
            include *
            autoLayout
        }
    }
}
