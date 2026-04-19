/*
  Архитектура: MariaDB/MySQL + клиентское C++ приложение (Example04).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Docker: SQL + клиент" "База данных и контейнер sql_test (Poco/MySQL client)" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "DKRSQL04"
        }

        student = person "Студент"

        sqldemo = softwareSystem "Демо доступа к SQL из контейнера" "СУБД в Docker-сети и приложение sql_test" {
            properties {
                cmdb "DKRSQL04"
            }

            db = container "MariaDB / MySQL" "Учебная БД archdb, пользователь stud" {
                tags "database" "mysql"
                technology "MariaDB (образ из mysql/)"
                properties {
                    entity "archdb"
                }
            }

            client = container "sql_test (C++/Poco)" "Клиент: подключение к БД по IP в docker-сети" {
                tags "api"
                technology "C++, Poco, MySQL client"
                url "file:./myserver"

                component "Проверка подключения" {
                    description "TCP к СУБД, выполнение запросов из учебного проекта"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }
            }

            client -> db "SQL-сессия" "TCP :3306" {
                properties {
                    send   "SQL-запросы"
                    return "Результаты выборок"
                }
            }
        }

        student -> sqldemo.client "Запускает сценарий (через compose)" "Docker"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Docker Compose (см. docker-compose.yml), сеть vpcbr" {
                properties {
                    type "docker-compose"
                }

                deploymentNode "db-node-1" "Контейнер СУБД (порт 3306)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance sqldemo.db
                }

                deploymentNode "sql_test" "Контейнер клиента (myserver)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance sqldemo.client
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

        systemContext sqldemo {
            include *
            autoLayout lr
        }

        container sqldemo {
            include *
            autoLayout lr
        }

        component sqldemo.client {
            include *
            autoLayout
        }

        dynamic sqldemo "001" {
            autoLayout
            title "Клиент обращается к БД в той же Docker-сети"
            student -> sqldemo.client "docker compose up"
            sqldemo.client -> sqldemo.db "TCP 3306 к db-node-1"
        }

        deployment sqldemo "Development" {
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
