# IR EXTENDER

![](docs/assets/ir_extender.svg)

L'IR Extender e' composto da due dispositivi:
* un dispositivo e' collegato ad un ricevitore infrarossi e alla rete Wi-Fi
* un altro dispositivo e' collegato ad un trasmettitore infrarossi e alla rete Wi-Fi
    > Le due reti Wi-Fi possono essere due reti differenti o la stessa rete.
* i dispositivi comunicano attraverso una piattaforma cloud.

I dispositivi utilizzano il protocollo MQTT per comunicare.

Il sistema e' formato da 3 componenti principali:
* Publisher MQTT: e' il dispositivo che ha il ricevitore infrarossi. 

    Il suo compito e':
    1. Ricevere un segnale infrarossi da un telecomando
    2. Inviare l'informazione del segnale infrarossi, utilizzando la rete Wi-Fi, con il protocollo MQTT al Broker MQTT

* Broker MQTT: e' un servizio fornito in cloud.

    Come fornitore del servizio abbiamo scelto [HiveMQ](https://www.hivemq.com/): non richiede carta di credito per registrarsi e ha una versione di prova gratuita.

    Il suo compito e':
    1. Ricevere l'informazione proveniente dal Publisher MQTT
    2. Inviare l'informazione MQTT al Subscriber MQTT

    Utilizzando un servizio in cloud non dobbiamo preoccuparci di configurarlo.

* Subscriber MQTT: e' il dispositivo che ha il trasmettitore infrarossi.

    Il suo compito e':
    1. Ricevere l'informazione dal Broker MQTT
    2. Inviare un segnale infrarossi con l'informazione del Broker MQTT con il trasmettitore infrarossi

I dispositivi sono schede NodeMCU per permettere di sviluppare velocemente e comodamente gli sketch arduino.

---

## Requisiti progetto

* [X] Completare il codice del receiver: fatto
* [X] Completare il codice del transmitter: fatto
* [X] Utilizzare una piattaforma cloud ([HiveMQ](https://www.hivemq.com/)): fatto
* [ ] Scrivere una documentazione in LaTeX: in corso
* [X] Creare schemi in fritzing per la documentazione: fatto

## Autori

- [Davide Pizzoli](https://github.com/pizidavi)
- [Stefano Zenaro](https://github.com/mario33881)
