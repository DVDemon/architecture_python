/*
  Архитектура: CQRS с Kafka (main.py — команды/запросы, writer.py — проекция в БД).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "CQRS + Kafka (FastAPI)" "Команды в топик user_topic; чтение из PostgreSQL" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "CQRSKFK"
        }

        client = person "Клиент API"

        cqrs = softwareSystem "CQRS демо" "Разделение команд (Kafka) и запросов (SQL)" {
            properties {
                cmdb "CQRSKFK"
            }

            api = container "Command & Query API" "main.py: POST /users → Kafka; GET → Postgres" {
                tags "api"
                technology "Python 3, FastAPI, SQLAlchemy, confluent-kafka"
                url "file:./main.py"
                properties {
                    external_name ext_cqrs_api
                }

                component "Публикация команд" {
                    description "События создания пользователя в user_topic"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "Синхронные запросы" {
                    description "GET /users читает актуальное состояние из БД"
                    properties {
                        type        capability
                        code        002
                        version     "1.0"
                    }
                }

                component "users_rest" "REST" {
                    properties {
                        type          api
                        protocol      rest
                    }
                }
            }

            writer = container "Projection writer" "writer.py: consumer → INSERT в Postgres" {
                tags "api"
                technology "Python 3, confluent-kafka, SQLAlchemy"
                url "file:./writer.py"

                component "Проекция в БД" {
                    description "Десериализация JSON из Kafka, запись User"
                    properties {
                        type        capability
                        code        003
                        version     "1.0"
                    }
                }
            }

            kafka = container "Apache Kafka" "Топик user_topic (внешний кластер)" {
                tags "web"
                technology "Kafka"
            }

            postgres = container "PostgreSQL" "Хранилище чтения и проекции" {
                tags "database" "postgresql"
                technology "PostgreSQL"
                properties {
                    entity "User"
                }
            }

            api -> kafka "produce user_topic" "TCP :9092"
            api -> postgres "SELECT для GET" "TCP :5432" {
                properties {
                    send   "SQL"
                    return "Пользователи"
                }
            }

            writer -> kafka "consume user_topic" "TCP :9092"
            writer -> kafka "доставка сообщений" "TCP"

            writer -> postgres "INSERT проекции" "TCP :5432" {
                properties {
                    send   "INSERT User"
                    return "OK"
                }
            }
        }

        client -> cqrs.api "HTTP API" "HTTP :8000"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Kafka и Postgres внешне; два Python-процесса" {
                properties {
                    type "mixed"
                }

                deploymentNode "api" "uvicorn main:app" {
                    properties {
                        type "process"
                    }
                    containerInstance cqrs.api
                }

                deploymentNode "writer" "python writer.py" {
                    properties {
                        type "process"
                    }
                    containerInstance cqrs.writer
                }

                deploymentNode "postgres" "СУБД" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance cqrs.postgres
                }

                deploymentNode "kafka" "Кластер брокеров" {
                    properties {
                        type "cluster"
                    }
                    containerInstance cqrs.kafka
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

        systemContext cqrs {
            include *
            autoLayout lr
        }

        container cqrs {
            include *
            autoLayout lr
        }

        component cqrs.api {
            include *
            autoLayout
        }

        component cqrs.writer {
            include *
            autoLayout
        }

        dynamic cqrs "001" {
            autoLayout
            title "Команда: POST /users → Kafka → writer → Postgres"
            client -> cqrs.api "POST /users/"
            cqrs.api -> cqrs.kafka "produce"
            cqrs.writer -> cqrs.kafka  "consume"
            cqrs.writer -> cqrs.postgres "INSERT"
        }

        dynamic cqrs "002" {
            autoLayout
            title "Запрос: GET из Postgres"
            client -> cqrs.api "GET /users/"
            cqrs.api -> cqrs.postgres "SELECT"
        }

        deployment cqrs "Development" {
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
