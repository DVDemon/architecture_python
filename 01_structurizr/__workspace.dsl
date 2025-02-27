workspace {
    name "Domodedovo airport"
    !identifiers hierarchical

    model {

        client = Person "Passanger"
        pilot  = Person "Pilot"
        security = Person "Security"
        dispatcher = Person "Flight Trafic Control"

        booking = softwareSystem "Flight Booking System"

        airport = softwareSystem "Domodedovo Airport" {
            -> booking "Get ticket info"


            duty_free = container "Duty Free"{
                technology "rust"
                tags "relax"

                component "Chip Drinks"
                component "Bar"
                component "Shop"
                component "Suvenirs"
                component "Perfuuume"
            }
            
            baggage = container "Baggage"{
                technology "python fastapi"
                tags "so-so"
            }

            customs = container "Customs" {
                technology "python fastapi"
                tags "anti_relax"
                -> baggage "check staff" "REST"
                -> duty_free "check staff" "REST"
            }

            gate_control = container "Gate Control"{
                technology "python fastapi"
                tags "anti_relax"
                -> baggage "Send baggage to flight" "REST"
                -> booking "Check ticket" "REST"
                
            }

            flight_crew = container "Flight Crew Automatization"{
                technology "python fastapi"
            }

            flight_schedule = container "Flight Schedule"{
                technology "python fastapi"
                -> flight_crew "Assign to flight" "REST"
            }

            chill = container "Chill & Relax"{
                technology "Java Spring Boot"
                tags "relax"
                -> customs "Register passanger" "REST"
            }

            flight_control = container "Flight Control System"{
                technology "C++ Userver"
                -> flight_schedule "Get schedule" "REST"
            }
        }

        client -> airport.chill "Check VIP passage" "REST"
        pilot -> airport.flight_crew "Register to the flight" "REST"

        client -> airport "Make trip"
        pilot  -> airport "Make trip possible"
        security -> airport "Make trip safe"
        dispatcher -> airport "Make flight controlable"

        deploymentEnvironment "PROD" {
            deploymentNode "DMZ" {
                deploymentNode "web-app.domodedovo.ru" {
                    containerInstance airport.chill
                }
            }

            deploymentNode "PROTECTED" {
                deploymentNode "k8.namespace" {
                    lb = infrastructureNode "LoadBalancer"

                    pod1 = deploymentNode "pod1" {
                        df = containerInstance airport.duty_free
                        instances 10
                    }
                    deploymentNode "pod2" {
                        containerInstance airport.baggage
                    }
                    deploymentNode "pod3" {
                        containerInstance airport.customs
                        containerInstance airport.gate_control
                    }
                    deploymentNode "pod4" {
                        containerInstance airport.flight_crew
                        containerInstance airport.flight_schedule
                    }

                    lb -> pod1.df "Send requests"
                }

            }
        }

    }

    views {

        themes default

        systemContext airport "context" {
            include *
            # autoLayout lr
        }

        container airport "c2" {
            include *
            autoLayout
        }

        container airport "c2relax" {
            include *
            exclude "element.tag==anti_relax"
            autoLayout
        }

        component airport.duty_free "c3" {
            include *
            autoLayout
        }

        deployment * "PROD" {
            include *
            autoLayout

        }

        dynamic airport "MyAlgorithm" "Some long text"{
           autoLayout lr
           client -> airport.chill "Круто оторваться с друзьями"
           airport.chill -> airport.customs "Нудно разуваться/обуваться"
           airport.customs -> airport.duty_free "Заняться самовосстановлением после таможни"
        }

    }
}