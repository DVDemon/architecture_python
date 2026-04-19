/*
  Архитектура: Apache httpd + статический контент (Example02).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Docker: Apache + статика" "Веб-сервер отдаёт HTML из смонтированного каталога" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "DKRAP02"
        }

        user = person "Пользователь"

        webdemo = softwareSystem "Статический сайт в Apache" "httpd и том с HTML из рабочей директории" {
            properties {
                cmdb "DKRAP02"
            }

            apache = container "Apache HTTP Server" "httpd 2.4, документы из volume" {
                tags "web"
                technology "Apache httpd 2.4"
                url "file:."

                component "Раздача статики" {
                    description "Отдача файлов из htdocs (bind-mount каталог примера)"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }
            }
        }

        user -> webdemo.apache "Открывает страницу в браузере" "HTTP :8080"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Docker Compose (см. docker-compose.yml)" {
                properties {
                    type "docker-compose"
                }

                deploymentNode "apache" "Контейнер httpd (порт 8080→80)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance webdemo.apache
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

        systemContext webdemo {
            include *
            autoLayout lr
        }

        container webdemo {
            include *
            autoLayout lr
        }

        dynamic webdemo "001" {
            autoLayout
            title "Запрос статической страницы"
            user -> webdemo.apache "GET /\nlocalhost:8080"
        }

        deployment webdemo "Development" {
            include *
            autoLayout
        }
    }
}
