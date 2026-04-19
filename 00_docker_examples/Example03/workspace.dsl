/*
  Архитектура: Nginx + статический контент (Example03).
  Структура и правила оформления — по workspace_template.dsl (ПТР).
*/
workspace "Docker: Nginx + статика" "Обратный прокси/веб-сервер отдаёт HTML из volume" {

    !identifiers hierarchical

    model {
        properties {
            workspace_cmdb "DKRNG03"
        }

        user = person "Пользователь"

        webdemo = softwareSystem "Статический сайт в Nginx" "nginx:latest и HTML из каталога примера" {
            properties {
                cmdb "DKRNG03"
            }

            nginx = container "Nginx" "Раздача статики из /usr/share/nginx/html" {
                tags "web"
                technology "Nginx"
                url "file:."

                component "Раздача статики" {
                    description "Файлы из bind-mount каталога примера"
                    properties {
                        type        capability
                        code        001
                        version     "1.0"
                    }
                }
            }
        }

        user -> webdemo.nginx "Открывает страницу в браузере" "HTTP :8080"

        deploymentEnvironment "Development" {
            deploymentNode "Хост разработки" "Docker Compose (см. docker-compose.yml)" {
                properties {
                    type "docker-compose"
                }

                deploymentNode "nginx" "Контейнер nginx (порт 8080→80)" {
                    properties {
                        type "docker-service"
                    }
                    containerInstance webdemo.nginx
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
            user -> webdemo.nginx "GET /\nlocalhost:8080"
        }

        deployment webdemo "Development" {
            include *
            autoLayout
        }
    }
}
