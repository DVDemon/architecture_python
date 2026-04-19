/*
  Архитектура: Poco REST + MongoDB + Redis (docker-compose.yaml).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Poco REST + Mongo + Redis" "API, документы и кэш в одном стенде" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "POCOREDIS"
        }

        student = person "Клиент API"

        app = softwareSystem "Poco Server (Mongo + Redis)" "Кэш Redis и основное хранилище MongoDB" {
            properties {
                cmdb "POCOREDIS"
            }

            api = container "REST API" "REDIS_HOST=cache, MONGO_HOST=mongodb" {
                tags "api"
                technology "C++17, POCO"
                url "file:./src"
                properties {
                    external_name ext_poco_redis_api
                }

                component "Кэш и данные" {
                    description "Чтение/запись в MongoDB, кэширование в Redis"
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

            mongo = container "MongoDB" "Основное хранилище" {
                tags "database" "mongodb"
                technology "MongoDB 5.0"
                properties {
                    entity "Документы приложения"
                }
            }

            redis = container "Redis" "Кэш / сессии / счётчики" {
                tags "database" "redis"
                technology "Redis 6.2"
                properties {
                    entity "Ключи кэша"
                }
            }

            api -> mongo "Персистентные данные" "TCP :27017" {
                properties {
                    send   "Запросы к данным"
                    return "Документы"
                }
            }

            api -> redis "Кэш" "TCP :6379" {
                properties {
                    send   "GET/SET"
                    return "Значения из кэша"
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

                deploymentNode "cache" "Контейнер Redis (6379)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance app.redis
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
            title "Запрос с использованием кэша и БД"
            student -> app.api "HTTP"
            app.api -> app.redis "Проверка кэша\n:6379"
            app.api -> app.mongo "Загрузка из БД\n:27017"
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
