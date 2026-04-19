/*
  Архитектура: Producer / Consumer через RabbitMQ (client.py, server.py).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "RabbitMQ (Python)" "Очередь user_queue: публикация и потребление JSON" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "RABBITDEMO"
        }

        operator = person "Оператор (запуск скриптов)"

        mqdemo = softwareSystem "Демо очередей RabbitMQ" "pika: producer и consumer + брокер" {
            properties {
                cmdb "RABBITDEMO"
            }

            broker = container "RabbitMQ" "Брокер AMQP (docker run или внешний)" {
                tags "web"
                technology "RabbitMQ"
                properties {
                    external_name ext_rabbit_broker
                }
            }

            producer = container "Producer" "client.py — публикация в user_queue" {
                tags "api"
                technology "Python 3, pika"
                url "file:./client.py"

                component "Публикация сообщений" {
                    description "JSON пользователей в очередь"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }
            }

            consumer = container "Consumer" "server.py — чтение и обработка" {
                tags "api"
                technology "Python 3, pika"
                url "file:./server.py"

                component "Обработка сообщений" {
                    description "Получение из user_queue, бизнес-логика"
                    properties {
                        type        capability
                        code        002
                        version     "1.0"
                    }
                }
            }

            producer -> broker "basic_publish" "AMQP :5672"
            consumer -> broker "Подключение и consume" "AMQP :5672"
            broker -> consumer "Доставка сообщений из user_queue" "AMQP" {
                properties {
                    send   "Сообщения из очереди"
                    return "ACK"
                }
            }
        }

        operator -> mqdemo.producer "Запуск producer"
        operator -> mqdemo.consumer "Запуск consumer"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Брокер в Docker, Python локально" {
                properties {
                    type "mixed"
                }

                deploymentNode "rabbitmq" "Контейнер брокера" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance mqdemo.broker
                }

                deploymentNode "scripts" "Два терминала: consumer затем producer" {
                    properties {
                        type "process"
                    }
                    containerInstance mqdemo.producer
                    containerInstance mqdemo.consumer
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

        systemContext mqdemo {
            include *
            autoLayout lr
        }

        container mqdemo {
            include *
            autoLayout lr
        }

        component mqdemo.producer {
            include *
            autoLayout
        }

        component mqdemo.consumer {
            include *
            autoLayout
        }

        dynamic mqdemo "001" {
            autoLayout
            title "Сообщение от producer к consumer через брокер"
            mqdemo.producer -> mqdemo.broker "publish"
            mqdemo.broker -> mqdemo.consumer "deliver"
        }

        deployment mqdemo "Development" {
            include *
            autoLayout
        }
    }
}
