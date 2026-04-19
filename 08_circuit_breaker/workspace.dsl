/*
  Архитектура: два микросервиса FastAPI и паттерн Circuit Breaker.
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Circuit Breaker (FastAPI)" "Order Service вызывает User Service через CB" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "CBREAKER"
        }

        client = person "Клиент"

        shop = softwareSystem "Демо устойчивых вызовов" "Два uvicorn-процесса: заказы и пользователи" {
            properties {
                cmdb "CBREAKER"
            }

            order = container "Order Service" "order_service.py, порт 8002, Circuit Breaker" {
                tags "api"
                technology "Python 3, FastAPI"
                url "file:./order_service.py"
                properties {
                    external_name ext_cb_order
                }

                component "Circuit Breaker" {
                    description "Состояния CLOSE / OPEN / SEMI_OPEN, пороги ошибок и таймеры"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "orders_api" "REST заказов" {
                    properties {
                        type          api
                        protocol      rest
                    }
                }
            }

            users = container "User Service" "user_service.py, порт 8001" {
                tags "api"
                technology "Python 3, FastAPI"
                url "file:./user_service.py"
                properties {
                    external_name ext_cb_users
                }

                component "Профиль пользователя" {
                    description "Данные пользователей для заказов"
                    properties {
                        type        capability
                        code        002
                        version     "1.0"
                    }
                }

                component "users_internal" "Внутренний REST" {
                    properties {
                        type          api
                        protocol      rest
                    }
                }
            }

            order -> users "HTTP при CLOSE/SEMI_OPEN" "HTTP :8001" {
                properties {
                    send   "Запросы к User Service"
                    return "Ответ или исключение для CB"
                }
            }
        }

        client -> shop.order "Создание заказов / запросы" "HTTP :8002"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Два процесса uvicorn (см. README)" {
                properties {
                    type "local"
                }

                deploymentNode "order_service" "Порт 8002" {
                    properties {
                        type "process"
                    }
                    containerInstance shop.order
                }

                deploymentNode "user_service" "Порт 8001" {
                    properties {
                        type "process"
                    }
                    containerInstance shop.users
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

        systemContext shop {
            include *
            autoLayout lr
        }

        container shop {
            include *
            autoLayout lr
        }

        component shop.order {
            include *
            autoLayout
        }

        component shop.users {
            include *
            autoLayout
        }

        dynamic shop "001" {
            autoLayout
            title "Заказ обращается к пользователям"
            client -> shop.order "HTTP"
            shop.order -> shop.users "Внутренний вызов\n(или OPEN — без вызова)"
        }

        deployment shop "Development" {
            include *
            autoLayout
        }
    }
}
