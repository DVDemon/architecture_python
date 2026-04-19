/*
  Архитектура: Poco REST + MongoDB (docker-compose.yaml).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Poco REST + MongoDB" "REST API и документное хранилище" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "POCOMONGO"
        }

        student = person "Клиент API"

        app = softwareSystem "Poco Server с MongoDB" "Учебный сервис: Mongo driver, JSON API" {
            properties {
                cmdb "POCOMONGO"
            }

            api = container "REST API" "MONGO_HOST=mongodb, база poco_template" {
                tags "api"
                technology "C++17, POCO, MongoDB C driver"
                url "file:./src"
                properties {
                    external_name ext_poco_mongo_api
                }

                component "Документная модель" {
                    description "Репозитории поверх MongoDB"
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
                    }
                }
            }

            mongo = container "MongoDB" "Персистентность документов" {
                tags "database" "mongodb"
                technology "MongoDB 5.0"
                properties {
                    entity "Коллекции учебного шаблона"
                }
            }

            api -> mongo "CRUD по коллекциям" "TCP :27017" {
                properties {
                    send   "BSON/запросы"
                    return "Документы"
                }
            }
        }

        student -> app.api "REST" "HTTP :8080"

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

                deploymentNode "mongodb" "Контейнер MongoDB (27017)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance app.mongo
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
            title "Запрос к API с обращением к MongoDB"
            student -> app.api "HTTP"
            app.api -> app.mongo "wire protocol\n:27017"
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
