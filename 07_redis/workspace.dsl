/*
  Архитектура: FastAPI + PostgreSQL + Redis (кэш чтения по ID).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Redis кэш (FastAPI)" "Двухуровневое хранение: Postgres и Redis" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "REDISCACHE"
        }

        client = person "Клиент API"

        app = softwareSystem "Сервис пользователей с кэшем" "FastAPI, SQLAlchemy, redis-py" {
            properties {
                cmdb "REDISCACHE"
            }

            api = container "REST API" "POST/GET пользователей, кэш на чтение по id" {
                tags "api"
                technology "Python 3, FastAPI"
                url "file:./main.py"
                properties {
                    external_name ext_rediscache_api
                }

                component "Кэширование чтений" {
                    description "GET /users/{id}: Redis затем Postgres, TTL"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "users_api" "REST" {
                    properties {
                        type          api
                        protocol      rest
                    }
                }
            }

            postgres = container "PostgreSQL" "Источник истины" {
                tags "database" "postgresql"
                technology "PostgreSQL"
                properties {
                    entity "User"
                }
            }

            redis = container "Redis" "Кэш" {
                tags "database" "redis"
                technology "Redis"
                properties {
                    entity "Сериализованные User по ключу id"
                }
            }

            api -> postgres "Запись и промахи кэша" "TCP :5432" {
                properties {
                    send   "SQL"
                    return "Строки"
                }
            }

            api -> redis "Чтение/запись кэша" "TCP :6379" {
                properties {
                    send   "GET/SET JSON"
                    return "Кэшированные объекты"
                }
            }
        }

        client -> app.api "HTTP" "HTTP :8000"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "uvicorn + внешние Postgres и Redis" {
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

                deploymentNode "redis" "Кэш" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance app.redis
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
            title "Чтение пользователя с кэшем"
            client -> app.api "GET /users/{id}"
            app.api -> app.redis "GET ключа"
            app.api -> app.postgres "При промахе SELECT"
            app.api -> app.redis "SET с TTL"
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
