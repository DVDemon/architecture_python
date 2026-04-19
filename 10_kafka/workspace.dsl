/*
  Архитектура: Kafka Producer / Consumer (client.py, server.py).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Kafka (Python)" "Топик topic_2: confluent-kafka producer и consumer" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "KAFKADEMO"
        }

        operator = person "Оператор (запуск скриптов)"

        kdemo = softwareSystem "Демо Kafka" "Producer, consumer и кластер брокеров" {
            properties {
                cmdb "KAFKADEMO"
            }

            cluster = container "Kafka brokers" "kafka1:9092, kafka2:9092 (или иной bootstrap)" {
                tags "web"
                technology "Apache Kafka"
                properties {
                    external_name ext_kafka_brokers
                }
            }

            producer = container "Producer" "client.py — запись в topic_2" {
                tags "api"
                technology "Python 3, confluent-kafka"
                url "file:./client.py"

                component "Публикация в топик" {
                    description "produce и flush"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }
            }

            consumer = container "Consumer" "server.py — чтение topic_2" {
                tags "api"
                technology "Python 3, confluent-kafka"
                url "file:./server.py"

                component "Потребление записей" {
                    description "poll, commit offsets"
                    properties {
                        type        capability
                        code        002
                        version     "1.0"
                    }
                }
            }

            producer -> cluster "Produce" "TCP :9092"
            consumer -> cluster "Subscribe / poll" "TCP :9092"
            // cluster -> consumer "Fetch записей" "TCP" {
            //     properties {
            //         send   "Сообщения topic_2"
            //         return "ACK offset"
            //     }
            // }
        }

        operator -> kdemo.producer "Запуск producer"
        operator -> kdemo.consumer "Запуск consumer"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Кластер Kafka вне репозитория; Python локально" {
                properties {
                    type "mixed"
                }

                deploymentNode "kafka" "Брокеры (внешняя инфраструктура)" {
                    properties {
                        type "cluster"
                    }
                    containerInstance kdemo.cluster
                }

                deploymentNode "scripts" "Два терминала: consumer затем producer" {
                    properties {
                        type "process"
                    }
                    containerInstance kdemo.producer
                    containerInstance kdemo.consumer
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

        systemContext kdemo {
            include *
            autoLayout lr
        }

        container kdemo {
            include *
            autoLayout lr
        }

        component kdemo.producer {
            include *
            autoLayout
        }

        component kdemo.consumer {
            include *
            autoLayout
        }

        dynamic kdemo "001" {
            autoLayout
            title "Сообщение producer → топик → consumer"
            kdemo.producer -> kdemo.cluster "produce topic_2"
            kdemo.consumer -> kdemo.cluster "fetch"
        }

        deployment kdemo "Development" {
            include *
            autoLayout
        }
    }
}
