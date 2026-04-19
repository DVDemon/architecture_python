/*
  Архитектура: примеры pymongo и replica set в Docker (cluster/docker-compose.yml).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "MongoDB (учебные примеры)" "Клиентские скрипты Python и трёхузловой replica set" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "MONGODEMO"
        }

        student = person "Студент"

        lab = softwareSystem "Лаборатория MongoDB" "Скрипты в репозитории и стенд в Docker" {
            properties {
                cmdb "MONGODEMO"
            }

            scripts = container "Python-клиенты" "pymongo: подключение, insert, find" {
                tags "api"
                technology "Python 3, pymongo"
                url "file:."

                component "Доступ к данным" {
                    description "Примеры работы с коллекциями и запросами"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }
            }

            mongo = container "MongoDB" "Replica set myReplicaSet (3× mongod)" {
                tags "database" "mongodb"
                technology "MongoDB 5.0"
                properties {
                    entity "Документы коллекций"
                }
            }

            scripts -> mongo "Драйвер wire protocol" "TCP :27017 (или проброшенные 27018–27020)" {
                properties {
                    send   "insert, find, команды rs"
                    return "Документы и статусы"
                }
            }
        }

        student -> lab.scripts "Запуск примеров и cluster/mongo_setup.sh"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Docker Compose в каталоге cluster/" {
                properties {
                    type "docker-compose"
                }

                deploymentNode "mongo1" "Узел replica set" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance lab.mongo
                }

                deploymentNode "mongo2" "Узел replica set" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance lab.mongo
                }

                deploymentNode "mongo3" "Узел replica set" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance lab.mongo
                }

                deploymentNode "mongosetup" "Инициализация rs (mongo_setup.sh)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance lab.mongo
                }

                deploymentNode "workstation" "Локальные Python-скрипты" {
                    properties {
                        type "local"
                    }
                    containerInstance lab.scripts
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

        systemContext lab {
            include *
            autoLayout lr
        }

        container lab {
            include *
            autoLayout lr
        }

        component lab.scripts {
            include *
            autoLayout
        }

        dynamic lab "001" {
            autoLayout
            title "Клиент обращается к MongoDB"
            student -> lab.scripts "python 01_example.py"
            lab.scripts -> lab.mongo "insert/find"
        }

        deployment lab "Development" {
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
