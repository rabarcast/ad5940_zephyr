# notificador_app

Proyecto Flutter para una app que recibe datos del microcontrolador y
los envía junto con otros datos que aportan contexto a una base de datos vía http.

[ Nordic nRF5340 ]
        │
        │  BLE (advertising / notify)
        ▼
[ APP FLUTTER ]
        │
        │  parsea datos
        ▼
        HTTP POST
        ▼
[ SERVIDOR ip:puerto ]
