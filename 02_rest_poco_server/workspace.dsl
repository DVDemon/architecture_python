/*
  Архитектура: REST-сервер на C++ / Poco (шаблон без внешней БД в compose).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Poco REST (базовый)" "HTTP API, JWT, Swagger, метрики — один контейнер" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "POCOREST"
        }

        student = person "Клиент API"

        pocoapp = softwareSystem "Poco Template Server" "Учебный REST-сервер на POCO C++ Libraries" {
            properties {
                cmdb "POCOREST"
            }

            api = container "REST API" "Маршруты hello, auth, users, metrics, OpenAPI" {
                tags "api"
                technology "C++17, POCO, CMake"
                url "file:./src"
                properties {
                    external_name ext_poco_rest_api
                }

                component "Обработка HTTP" {
                    description "Маршрутизация, JSON, обработчики REST"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "public_api" "Публичные и защищённые эндпоинты" {
                    properties {
                        type          api
                        api_url       "http://localhost:8080/"
                        protocol      rest
                        "GET /hello"  rps=50;latency=100;error_rate=0.01
                    }
                }
            }
        }

        student -> pocoapp.api "Вызовы REST, Swagger UI" "HTTP :8080"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Docker Compose (см. docker-compose.yaml)" {
                properties {
                    type "docker-compose"
                }

                deploymentNode "poco_template_server" "Контейнер приложения (порт 8080)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance pocoapp.api
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

        systemContext pocoapp {
            include *
            autoLayout lr
        }

        container pocoapp {
            include *
            autoLayout lr
        }

        component pocoapp.api {
            include *
            autoLayout
        }

        dynamic pocoapp "001" {
            autoLayout
            title "Запрос к REST API"
            student -> pocoapp.api "GET /hello\n:8080"
        }

        deployment pocoapp "Development" {
            include *
            autoLayout
        }
    }
}
