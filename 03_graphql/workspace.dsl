/*
  Архитектура: GraphQL-сервер на C++ (cppgraphqlgen + POCO HTTP), PostgreSQL.
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "GraphQL на C++" "HTTP :8080, схема GraphQL, резолверы и БД" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "GQLCPP"
        }

        student = person "Клиент (браузер / curl)"

        gqlsys = softwareSystem "GraphQL учебный сервис" "cppgraphqlgen, Poco::Net, PostgreSQL" {
            properties {
                cmdb "GQLCPP"
            }

            server = container "GraphQL HTTP Server" "Парсинг запроса, resolve, JSON-ответ" {
                tags "api"
                technology "C++17, POCO, cppgraphqlgen"
                url "file:./web_server"

                component "GraphQL execution" {
                    description "parseString → service->resolve → JSON"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }

                component "graphql_http" "POST GraphQL" {
                    properties {
                        type          api
                        protocol      graphql
                        "POST /"      rps=30;latency=200;error_rate=0.01
                    }
                }
            }

            db = container "PostgreSQL" "Подключение через конфиг (host, port, БД)" {
                tags "database" "postgresql"
                technology "PostgreSQL"
                url "file:./database"
                properties {
                    entity "Предметная область схемы GraphQL"
                }
            }

            server -> db "Загрузка и изменение данных" "TCP SQL" {
                properties {
                    send   "SQL"
                    return "Строки для маппинга в GraphQL"
                }
            }
        }

        student -> gqlsys.server "GraphQL over HTTP" "HTTP :8080"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Сборка CMake, локально или в CI" {
                properties {
                    type "local"
                }

                deploymentNode "graphql-server" "Процесс HTTPWebServer" {
                    properties {
                        type "process"
                    }
                    containerInstance gqlsys.server
                }

                deploymentNode "db-host" "PostgreSQL (внешний к процессу)" {
                    properties {
                        type "vm"
                    }
                    containerInstance gqlsys.db
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

        systemContext gqlsys {
            include *
            autoLayout lr
        }

        container gqlsys {
            include *
            autoLayout lr
        }

        component gqlsys.server {
            include *
            autoLayout
        }

        dynamic gqlsys "001" {
            autoLayout
            title "GraphQL-запрос с обращением к БД"
            student -> gqlsys.server "POST { query }"
            gqlsys.server -> gqlsys.db "SELECT/UPDATE"
        }

        deployment gqlsys "Development" {
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
