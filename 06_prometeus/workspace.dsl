/*
  Архитектура: FastAPI + экспорт метрик Prometheus (порт 9100) и API (8000).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Prometheus метрики (FastAPI)" "Счётчики и гистограммы latency; отдельный Prometheus scrape" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "PROMFAST"
        }

        user = person "Пользователь API"
        operator = person "SRE / наблюдатель"

        obs = softwareSystem "Демо метрик приложения" "FastAPI, prometheus_client и сервер Prometheus" {
            properties {
                cmdb "PROMFAST"
            }

            api = container "FastAPI" "Бизнес-эндпоинты /hello, /goodbye" {
                tags "api"
                technology "Python 3, FastAPI"
                url "file:./main.py"
                properties {
                    external_name ext_promfast_http
                }

                component "HTTP middleware" {
                    description "Счётчик http_requests_total и гистограмма latency по method/endpoint"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "business_http" "Публичные GET" {
                    properties {
                        type          api
                        protocol      rest
                    }
                }
            }



            prometheus = container "Prometheus Server" "scrape_configs → targets приложения :9100" {
                tags "web"
                technology "Prometheus"
            }

            prometheus -> api "Scrape /metrics" "HTTP :9100"
        }

        user -> obs.api "Вызовы API" "HTTP :8000"
        operator -> obs.prometheus "Конфигурация scrape, PromQL" "HTTP :9090"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Локальные процессы (см. README)" {
                properties {
                    type "local"
                }

                deploymentNode "app" "Один процесс Python: uvicorn + HTTP-сервер метрик" {
                    properties {
                        type "process"
                    }
                    containerInstance obs.api
                }

                deploymentNode "prometheus" "Отдельный процесс Prometheus" {
                    properties {
                        type "process"
                    }
                    containerInstance obs.prometheus
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

        systemContext obs {
            include *
            autoLayout lr
        }

        container obs {
            include *
            autoLayout lr
        }

        component obs.api {
            include *
            autoLayout
        }

        dynamic obs "001" {
            autoLayout
            title "HTTP-запрос и сбор метрик"
            user -> obs.api "GET /hello\n:8000"
            obs.prometheus -> obs.api "scrape\n:9100"
        }

        deployment obs "Development" {
            include *
            autoLayout
        }
    }
}
