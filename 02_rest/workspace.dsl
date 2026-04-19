/*
  Архитектура: примеры REST на FastAPI (01_simple, 02_user, 03_jwt).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "REST примеры (FastAPI)" "От Hello World до CRUD и JWT" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "RESTPY"
        }

        developer = person "Разработчик"

        restlab = softwareSystem "Лаборатория REST" "Несколько учебных приложений uvicorn на одном порту по очереди" {
            properties {
                cmdb "RESTPY"
            }

            api = container "FastAPI приложение" "Запуск выбранного модуля: 01_simple / 02_user / 03_jwt" {
                tags "api"
                technology "Python 3, FastAPI, Uvicorn"
                url "file:."
                properties {
                    external_name ext_restpy_api
                }

                component "REST-слой" {
                    description "Маршруты, модели Pydantic, при необходимости JWT (python-jose)"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "openapi" "Swagger / OpenAPI" {
                    properties {
                        type          api
                        api_url       "http://localhost:8000/docs"
                        protocol      rest
                    }
                }
            }
        }

        developer -> restlab.api "Запуск и вызовы" "HTTP :8000"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Локальный uvicorn" {
                properties {
                    type "local"
                }

                deploymentNode "fastapi" "Процесс с выбранным файлом приложения" {
                    properties {
                        type "process"
                    }
                    containerInstance restlab.api
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

        systemContext restlab {
            include *
            autoLayout lr
        }

        container restlab {
            include *
            autoLayout lr
        }

        component restlab.api {
            include *
            autoLayout
        }

        dynamic restlab "001" {
            autoLayout
            title "Обращение к учебному API"
            developer -> restlab.api "GET/POST\n:8000/docs"
        }

        deployment restlab "Development" {
            include *
            autoLayout
        }
    }
}
