#ifdef USE_ROLLERSHUTTER

#warning **** DEF XDRV_91 ****

#define XDRV_91             91

#include <xdrv_91_rollershutter.h>

enum RollerShutterCommands { CMND_RS_AC, CMND_RS_PC };
const char kRollerShutterCommands[] PROGMEM = D_CMND_RS_AC "|" D_CMND_RS_PC;

void RollerShutterInit(){
  snprintf_P(log_data, sizeof(log_data), "RollerShutter Init");
  AddLog(LOG_LEVEL_INFO);

  ExecuteCommandPower(1, POWER_OFF_NO_STATE, SRC_BUTTON); // SRC_BUTTON COMO ORIGEN PARA QUE SE TRATE COMO ORDEN INTERNA
  ExecuteCommandPower(2, POWER_OFF_NO_STATE, SRC_BUTTON);
  ExecuteCommandPower(3, POWER_OFF_NO_STATE, SRC_BUTTON);
  ExecuteCommandPower(4, POWER_OFF_NO_STATE, SRC_BUTTON);
}

// Subscribe to percentage initial states, so that mqtt inits my position at start with last persistent mqtt value stored
void RollerShutterMqttSubscribe() {
  char stopic[TOPSZ];

  if(PER_NUM_PERSIANAS > 0){
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, "RollerShutterPercent1"); // Suscribo el porcentaje de la persiana 1
    MqttSubscribe(stopic);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Suscrito a valor ini MQTT => %s"), stopic);
    AddLog(LOG_LEVEL_INFO);
  }
  if(PER_NUM_PERSIANAS > 1){
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, "RollerShutterPercent2"); // Suscribo el porcentaje de la persiana 2
    MqttSubscribe(stopic);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Suscrito a valor ini MQTT => %s"), stopic);
    AddLog(LOG_LEVEL_INFO);
  }
}

// mqtt initial value persistent value received
void rsInitMqttValue(int rs_index, byte valor) {
  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Recibido PCT ini MQTT, elimino suscripcion: %d"), rs_index, valor);
  AddLog(LOG_LEVEL_INFO);

  if(!inicializado[rs_index-1]){
    if(valor >= 100){
      // El total del recorrido mas el de las lamas
      perTiempoPosicionActual[rs_index-1] = per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1];
    }else{
      // El proporcional al recorrido solamente
      perTiempoPosicionActual[rs_index-1] = porcentajeATiempo(rs_index, valor);
    }
    // Lo marco como inicializado
    inicializado[rs_index-1] = true;

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Recibido PCT ini MQTT, elimino suscripcion: %d"), rs_index, valor);
    AddLog(LOG_LEVEL_INFO);
  }
  // Quito la suscripcion al porcentaje
  char stopic[TOPSZ];
  GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterPercent1" : "RollerShutterPercent2");
  MqttClient.unsubscribe(stopic);
}

// Funcion llamada siempre al recibir un comando para asegurar que he inicializado valores
void inicializar(int rs_index) {
    if(!inicializado[rs_index-1]){
      // No se ha inicializado aun por mqtt, lo hago manualmente
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Inicializo a mano"), rs_index);
      AddLog(LOG_LEVEL_INFO);
      // No he podido obtener de mqtt los datos iniciales pero necesito empezar, se los pongo yo
      perTiempoPosicionActual[rs_index-1] = 0;
      inicializado[rs_index-1] = true;
      // Quito la suscripcion al porcentaje mqtt
      char stopic[TOPSZ];
      GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterPercent1" : "RollerShutterPercent2");
      MqttClient.unsubscribe(stopic);
    }
}

// Metodo al que se llama al detectar una pulsacion de boton fisico del sonoff
// Lo uso para poder accionar el motor manualmente
void RS_Boton(int btn_index, int hold_btn){
  // Miro a que persiana corresponde
  switch(btn_index){
    case 1:
      if(perState[0] == RS_IDLE){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Boton pulsado: UP. Subir"));
        AddLog(LOG_LEVEL_INFO);
        RSUp(1);
      }else{
        snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Boton pulsado: UP. Parar"));
        AddLog(LOG_LEVEL_INFO);
        RSStop(1);
      }
      break;
    case 2:
      if(perState[0] == RS_IDLE){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Boton pulsado: DOWN. Bajar"));
        AddLog(LOG_LEVEL_INFO);
        RSDown(1);
      }else{
        snprintf_P(log_data, sizeof(log_data), PSTR("RS1: Boton pulsado: DOWN. Parar"));
        AddLog(LOG_LEVEL_INFO);
        RSStop(1);
      }
      break;
    case 3:
      if(perState[1] == RS_IDLE){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Boton pulsado: UP. Subir"));
        AddLog(LOG_LEVEL_INFO);
        RSUp(2);
      }else{
        snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Boton pulsado: UP. Parar"));
        AddLog(LOG_LEVEL_INFO);
        RSStop(2);
      }
      break;
    case 4:
      if(perState[1] == RS_IDLE){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Boton pulsado: DOWN. Bajar"));
        AddLog(LOG_LEVEL_INFO);
        RSDown(2);
      }else{
        snprintf_P(log_data, sizeof(log_data), PSTR("RS2: Boton pulsado: DOWN. Parar"));
        AddLog(LOG_LEVEL_INFO);
        RSStop(2);
      }
      break;
  }
}

// Metodo llamado en cada ciclo del loop de tasmota
void RollerShutterHandler(){
  if(PER_NUM_PERSIANAS>0){
    RollerShutterHandler(1);
  }
  if(PER_NUM_PERSIANAS>1){
    RollerShutterHandler(2);
  }
}

void RollerShutterHandler(int rs_index){
  unsigned long now = millis();

  // Si esta en movimiento
  if(perState[rs_index-1] != RS_IDLE){
    // Miro si ya he llegado al objetivo
    if((now - perTiempoInicioMotor[rs_index-1]) >= perTiempoAMoverMotor[rs_index-1]){
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Objetivo alcanzado tiempoMoviendo:%d tiempoAMover:%d"), rs_index, (now - perTiempoInicioMotor[rs_index-1]), perTiempoAMoverMotor[rs_index-1]);
      AddLog(LOG_LEVEL_DEBUG);

      // Actualizo posicion actual antes de poner el estado a IDLE
      actualizarTiempoPosicionActual(rs_index);

      // Actualizo estado
      perState[rs_index-1] = RS_IDLE;
      // Indico que ha cambiado el estado para que se actualice en la proxima iteracion
      cambiadoEstado[rs_index-1] = true;
    }else{
      // No ha llegado al Objetivo
      if(ciclosSerial == 50){
        actualizarTiempoPosicionActual(rs_index);
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: transcurrido:%d total:%d posActual:%d"), rs_index, (now - perTiempoInicioMotor[rs_index-1]), perTiempoAMoverMotor[rs_index-1], perTiempoPosicionActual[rs_index-1]);
        AddLog(LOG_LEVEL_DEBUG);
        ciclosSerial = 0;
      }else{
        ciclosSerial++;
      }
    }
  }

  // Si acaba de cambiar de estado
  if(cambiadoEstado[rs_index-1]){
    if(perState[rs_index-1] == RS_IDLE){
      // Parado, ha llegado al objetivo o se le ha mandado parar

      // Apago ambos reles sin enviar el cambio de estado a mqtt
      ExecuteCommandPower(persiana_down_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);
      ExecuteCommandPower(persiana_up_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Cambiado estado a IDLE. Pos actual:%d PCTactual:%d Envio retained"), rs_index, perTiempoPosicionActual[rs_index-1], tiempoAPorcentaje(rs_index, perTiempoPosicionActual[rs_index-1]));
      AddLog(LOG_LEVEL_INFO);

      // Almaceno valor en mqtt como retained
      perSendPorcentaje(rs_index, true);
      // Mando info del estado
      perSendEstado(rs_index);
    }else if(perState[rs_index-1] == RS_UP){
      // Subiendo

      // Hago los cambios en los reles sin enviar el cambio de estado a mqtt
      ExecuteCommandPower(persiana_down_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);
      ExecuteCommandPower(persiana_up_actuator[rs_index-1], POWER_ON_NO_STATE, SRC_BUTTON);

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Cambiado estado a UP"), rs_index);
      AddLog(LOG_LEVEL_INFO);

      // Mando info del estado
      perSendEstado(rs_index);
    }else if(perState[rs_index-1] == RS_DOWN){
      // Bajando

      // Hago los cambios en los reles sin enviar el cambio de estado a mqtt
      ExecuteCommandPower(persiana_up_actuator[rs_index-1], POWER_OFF_NO_STATE, SRC_BUTTON);
      ExecuteCommandPower(persiana_down_actuator[rs_index-1], POWER_ON_NO_STATE, SRC_BUTTON);

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Cambiado estado a DOWN"), rs_index);
      AddLog(LOG_LEVEL_INFO);

      // Mando info del estado
      perSendEstado(rs_index);
    }
    // Desmarco el cambio de estado
    cambiadoEstado[rs_index-1] = false;
  }
}

void RSStop(int rs_index){
  inicializar(rs_index);
  unsigned long now = millis();

  if(perState[rs_index-1] != RS_IDLE){
    // Solicitado PARAR y no estaba ya parado

    // Indico que ya he llegado al objetivo para que pare
    perTiempoAMoverMotor[rs_index-1] = now - perTiempoInicioMotor[rs_index-1];

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado parar"), rs_index);
    AddLog(LOG_LEVEL_INFO);

    // Llamo inmediatamente a la funcion que lo trata, en lugar de esperar a otro ciclo
    RollerShutterHandler(rs_index);
  }else{
    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado parar pero ya parado, no hago nada. Reenvio info estado"), rs_index);
    AddLog(LOG_LEVEL_INFO);

    // Como parece que el controlador no estaba al tanto de mi estado, vuelvo a mandar toda la info de estado y pct
    perSendStatus(rs_index);
  }
}

void RSUp(int rs_index){
  inicializar(rs_index);
  unsigned long now = millis();

  // Actualizo la posicion actual (si estaba parado no recalcula, si estaba en movimiento actualiza la posicion)
  actualizarTiempoPosicionActual(rs_index);

  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado subir. posActual:%d estadoActual:%d"), rs_index, perTiempoPosicionActual[rs_index-1], perState[rs_index-1]);
  AddLog(LOG_LEVEL_INFO);

  // Miro si no esta ya arriba y (no esta subiendo o esta subiendo pero a una posicion intermedia)
  if((perTiempoPosicionActual[rs_index-1] > 0) && (perState[rs_index-1] != RS_UP || ((perTiempoPosicionInicial[rs_index-1] - perTiempoAMoverMotor[rs_index-1]) > 0))){
    // Tiempo actual como inicio de motor
    perTiempoInicioMotor[rs_index-1] = now;
    // Tiempo actual como inicio de motor
    perTiempoPosicionInicial[rs_index-1] = perTiempoPosicionActual[rs_index-1];

    // El tiempo necesario para mover motor es la posicion actual
    perTiempoAMoverMotor[rs_index-1] = perTiempoPosicionActual[rs_index-1];
    // Miro si parto de una posicion intermedia
    if(perTiempoPosicionActual[rs_index-1] < (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1])){
      // Parto de pos intermedia
      perTiempoAMoverMotor[rs_index-1] = perTiempoAMoverMotor[rs_index-1] + per_tiempo_ajuste_recorrido[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Parto desde pos intermedia, añado ajuste"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
    }else{
      // Parto desde abajo del todo
      perTiempoAMoverMotor[rs_index-1] = perTiempoAMoverMotor[rs_index-1] +per_tiempo_adicional_lamas[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Parto desde abajo, añado lamas"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
    }

    // Motor subiendo si no lo estaba ya
    if(perState[rs_index-1] != RS_UP){
      perState[rs_index-1] = RS_UP;
      cambiadoEstado[rs_index-1] = true;
    }

    // Llamo inmediatamente a la funcion que lo trata, en lugar de esperar a otro ciclo
    RollerShutterHandler(rs_index);
  }else{
    #ifdef SERIAL_OUTPUT
      if(perTiempoPosicionActual[rs_index-1] == 0){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Ya arriba, no hago nada"), rs_index);
        AddLog(LOG_LEVEL_INFO);
      }
      if((perTiempoPosicionInicial[rs_index-1] - perTiempoAMoverMotor[rs_index-1]) <= 0){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Ya objetivo 0, no hago nada"), rs_index);
        AddLog(LOG_LEVEL_INFO);
      }
    #endif
    // Como parece que el controlador no estaba al tanto de mi estado, lo vuelvo a mandar
    perSendStatus(rs_index);
  }
}

void RSDown(int rs_index){
  inicializar(rs_index);
  unsigned long now = millis();

  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado bajar. posActual:%d estadoActual:%d"), rs_index, perTiempoPosicionActual[rs_index-1], perState[rs_index-1]);
  AddLog(LOG_LEVEL_INFO);

  // Actualizo la posicion actual (si estaba parado no recalcula, si estaba en movimiento actualiza la posicion)
  actualizarTiempoPosicionActual(rs_index);

  if(
      (perTiempoPosicionActual[rs_index-1] < (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1])) && // No esta ya abajo
      (perState[rs_index-1] != RS_DOWN || ((perTiempoPosicionInicial[rs_index-1] + perTiempoAMoverMotor[rs_index-1]) < (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1]))) // El objetivo no es abajo del todo
    ){
    // No esta ya abajo y (no esta bajando o esta bajando pero no hasta abajo del todo)

    // Tiempo actual como inicio de motor
    perTiempoInicioMotor[rs_index-1] = now;
    // Tiempo actual como inicio de motor
    perTiempoPosicionInicial[rs_index-1] = perTiempoPosicionActual[rs_index-1];

    // El tiempo necesario para mover motor es el total - la posicion actual
    perTiempoAMoverMotor[rs_index-1] = (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1])  - perTiempoPosicionActual[rs_index-1];
    if(perTiempoPosicionActual[rs_index-1] > 0){
      // Parto de posicion intermedia, añado ajuste
      perTiempoAMoverMotor[rs_index-1] = perTiempoAMoverMotor[rs_index-1] + per_tiempo_ajuste_recorrido[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Parto de pos intermedia, añado ajuste"), rs_index);
      AddLog(LOG_LEVEL_DEBUG);
    }

    // Motor bajando si no lo estaba ya
    if(perState[rs_index-1] != RS_DOWN){
      perState[rs_index-1] = RS_DOWN;
      cambiadoEstado[rs_index-1] = true;
    }

    // Llamo inmediatamente a la funcion que lo trata, en lugar de esperar a otro ciclo
    RollerShutterHandler(rs_index);
  }else{
    #ifdef SERIAL_OUTPUT
      if(perTiempoPosicionActual[rs_index-1] >= (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1])){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Ya abajo, no hago nada. Reenvio estado MQTT"), rs_index);
        AddLog(LOG_LEVEL_INFO);
      }
      if((perTiempoPosicionInicial[rs_index-1] + perTiempoAMoverMotor[rs_index-1]) >= (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1])){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Objetivo ya era 100, no hago nada. Reenvio estado MQTT"), rs_index);
        AddLog(LOG_LEVEL_INFO);
      }
    #endif
    if(perTiempoPosicionActual[rs_index-1] >= (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1])){
      // Como parece que el controlador no estaba al tanto de mi estado, lo vuelvo a mandar
      perSendStatus(rs_index);
    }
  }
}

void RSUpdateInfo(int rs_index){
  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado estado actual. Lo envio a MQTT"), rs_index);
  AddLog(LOG_LEVEL_INFO);
  perSendStatus(rs_index);
}

void RSPercent(int rs_index, byte valor) {
  inicializar(rs_index);
  unsigned long now = millis();

  // Actualizo pos actual en caso de que se este moviendo
  actualizarTiempoPosicionActual(rs_index);
  int porcentajeActual = tiempoAPorcentaje(rs_index, perTiempoPosicionActual[rs_index-1]);
  int porcentajeSolicitado = valor;
  unsigned long tiempoPosicionSolicitada = porcentajeATiempo(rs_index, porcentajeSolicitado);

  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado porcentaje, solicitado:%d actual:%d posicion sol.:%d posicion actual:%d"), rs_index, porcentajeSolicitado, porcentajeActual, tiempoPosicionSolicitada, perTiempoPosicionActual[rs_index-1]);
  AddLog(LOG_LEVEL_INFO);

  // Miro si ya estoy comparando porcentajes a grosso modo
  if(porcentajeActual != porcentajeSolicitado){
    // No estoy en el porcentaje solicitado
    // Pongo el tiempo inicial
    perTiempoInicioMotor[rs_index-1] = now;
    // Pongo la posicion inicial
    perTiempoPosicionInicial[rs_index-1] = perTiempoPosicionActual[rs_index-1];
    // Miro si tengo que ir arriba o abajo
    if(porcentajeSolicitado < porcentajeActual){
      // Tengo que subir
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Subiendo"), rs_index);
      AddLog(LOG_LEVEL_INFO);

      // Pongo el tiempo que necesito
      // Si estoy abajo del todo ya incluye las lamas
      perTiempoAMoverMotor[rs_index-1] = perTiempoPosicionActual[rs_index-1] - tiempoPosicionSolicitada;

      if(porcentajeSolicitado == 0 && porcentajeActual < 100){
        // Pos intermedia, añado el ajuste de recorrido
        perTiempoAMoverMotor[rs_index-1] = perTiempoAMoverMotor[rs_index-1] + per_tiempo_ajuste_recorrido[rs_index-1];
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado arriba del todo desde posicion intermedia. Añado ajuste"), rs_index);
        AddLog(LOG_LEVEL_DEBUG);
      }

      // Cambio el estado si no estaba ya
      if(perState[rs_index-1] != RS_UP){
        perState[rs_index-1] = RS_UP;
        cambiadoEstado[rs_index-1] = true;
      }
    }else{
      // Tengo que bajar
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Bajando"), rs_index);
      AddLog(LOG_LEVEL_INFO);

      // Pongo el tiempo que necesito
      perTiempoAMoverMotor[rs_index-1] = tiempoPosicionSolicitada - perTiempoPosicionActual[rs_index-1];
      if(porcentajeSolicitado == 100){
        // Añado el ajuste de lamas
        perTiempoAMoverMotor[rs_index-1] = perTiempoAMoverMotor[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1];
        // Miro si parto de posicion intermedia
        if(perTiempoPosicionActual[rs_index-1] > 0){
          // Añado el ajuste de lamas
          perTiempoAMoverMotor[rs_index-1] = perTiempoAMoverMotor[rs_index-1] + per_tiempo_ajuste_recorrido[rs_index-1];

          snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado abajo del todo desde posicion intermedia. Añado ajuste"), rs_index);
          AddLog(LOG_LEVEL_DEBUG);
        }
      }

      // Cambio el estado si no estaba ya
      if(perState[rs_index-1] != RS_DOWN){
        perState[rs_index-1] = RS_DOWN;
        cambiadoEstado[rs_index-1] = true;
      }
    }
    // Llamo inmediatamente a la funcion que lo trata, en lugar de esperar a otro ciclo
    RollerShutterHandler(rs_index);
  }else{ // Ya estoy en el solicitado
    // Parar
    if(perState[rs_index-1] != RS_IDLE){ // Miro si no estaba ya parado
      // Indico que ya he llegado al objetivo para que pare
      perTiempoAMoverMotor[rs_index-1] = now - perTiempoInicioMotor[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Solicitado mismo porcentaje en el que estoy y no estaba parado. Paro"), rs_index);
      AddLog(LOG_LEVEL_INFO);
      // Llamo inmediatamente a la funcion que lo trata, en lugar de esperar a otro ciclo
      RollerShutterHandler(rs_index);
    }
  }
}

// Envia el estado actual de toda la persiana
void perSendStatus(int rs_index){
	// Los estados del motor
  perSendEstado(rs_index);
	// El porcentaje
  perSendPorcentaje(rs_index);
}

// Envia el estado actual del toda la persiana
void perSendEstado(int rs_index){
  unsigned long currentMillis = millis();
  // Los estados del motor
  if(!perInitial_state_sent[rs_index-1] || ((perState[rs_index-1] != perStateEnviado[rs_index-1]) && (currentMillis - perUltimoEnvioEstado[rs_index-1] > PER_TIEMPO_ENTRE_ENVIOS_ESTADO))){
    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Envio estado a MQTT: %d"), rs_index, perState[rs_index-1]);
    AddLog(LOG_LEVEL_INFO);

    // Actualizo estado en MQTT
    char stopic[TOPSZ];
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%d"), perState[rs_index-1]); // Pongo el valor
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterAction1" : "RollerShutterAction2"); // Publico el estado
    MqttPublish(stopic);

    perUltimoEnvioEstado[rs_index-1] = currentMillis;
    perStateEnviado[rs_index-1] = perState[rs_index-1];
  }
}

// Envia el estado actual del toda la persiana
void perSendPorcentaje(int rs_index){
  perSendPorcentaje(rs_index, false);
}

void perSendPorcentaje(int rs_index, boolean retained){
  unsigned long currentMillis = millis();
  int pctActual = tiempoAPorcentaje(rs_index, perTiempoPosicionActual[rs_index-1]);
  // Miro si es retained o no lo he inicializado o (ha cambiado y ha pasado el tiempo min entre envios)
  if(!perInitial_state_sent[rs_index-1] || retained || ((perPorcentajeUltimoEnviado[rs_index-1] != pctActual) && (currentMillis - perUltimoEnvioPorcentaje[rs_index-1] > PER_TIEMPO_ENTRE_ENVIOS_PORCENTAJE))){
    #ifdef SERIAL_OUTPUT
      if(retained){
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Envio porcentaje a MQTT (retained): %d"), rs_index, pctActual);
        AddLog(LOG_LEVEL_INFO);
      }else{
        snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Envio porcentaje a MQTT: %d"), rs_index, pctActual);
        AddLog(LOG_LEVEL_DEBUG);
      }
    #endif
    // Actualizo estado en MQTT
    char stopic[TOPSZ];
    snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%d"), pctActual); // Configuro la cadena
    GetTopic_P(stopic, STAT, Settings.mqtt_topic, (rs_index == 1)? "RollerShutterPercent1" : "RollerShutterPercent2"); // Publico el estado
    MqttPublish(stopic, retained);

    perUltimoEnvioPorcentaje[rs_index-1] = currentMillis;
    perPorcentajeUltimoEnviado[rs_index-1] = pctActual;
  }
}

uint8_t tiempoAPorcentaje(int rs_index, unsigned long tiempo){
  uint8_t porc = (uint8_t)(tiempo*100/per_tiempo_total_recorrido[rs_index-1]);
  if(porc > 100) porc = 100;
  if(porc < 0) porc = 0;
  return porc;
}

// Devuelve la conversion de porcentaje a tiempo, y si es 100 o mas incluye el desfase de la persiana acumulada abajo
unsigned long porcentajeATiempo(int rs_index, uint8_t porcentaje){
  unsigned long valor;
  if(porcentaje < 0){
    valor = 0;
  }else if(valor >= 100){
    valor = (unsigned long) per_tiempo_total_recorrido[rs_index-1];
  }else{
    valor = (unsigned long) (per_tiempo_total_recorrido[rs_index-1]*porcentaje/100);
  }
  snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Porcentaje a Tiempo: porc:%d tiempo:%d tiempoTotRecorrido:%d"), rs_index, porcentaje, valor, per_tiempo_total_recorrido[rs_index-1]);
  AddLog(LOG_LEVEL_DEBUG_MORE);
  return valor;
}

// Actualiza la variable perTiempoPosicionActual en caso de estar subiendo o bajando. Si esta parado no hace nada
// Es importante justo antes de pasar a estado parado llamar a esta funcion para actualizar su valor
void actualizarTiempoPosicionActual(int rs_index){
  unsigned long now = millis();
  if(perState[rs_index-1] == RS_UP){
    // Estaba subiendo
    // Calculo la posicion actual = pos inicial menos el tiempo que se ha movido
    perTiempoPosicionActual[rs_index-1] = perTiempoPosicionInicial[rs_index-1] - (now - perTiempoInicioMotor[rs_index-1]);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Pos act calculada subiendo:%d"), rs_index, perTiempoPosicionActual[rs_index-1]);
    AddLog(LOG_LEVEL_DEBUG_MORE);
    // Corrijo
    if(perTiempoPosicionActual[rs_index-1] < 0){
      perTiempoPosicionActual[rs_index-1] = 0;

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Corrijo a cero:%d"), rs_index, perTiempoPosicionActual[rs_index-1]);
      AddLog(LOG_LEVEL_DEBUG_MORE);
    }
  }else if(perState[rs_index-1] == RS_DOWN){
    // Estaba bajando
    // Calculo la posicion actual = pos inicial mas el tiempo que se ha movido
    perTiempoPosicionActual[rs_index-1] = perTiempoPosicionInicial[rs_index-1] + (now - perTiempoInicioMotor[rs_index-1]);

    snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Pos act calculada bajando: %d"), rs_index, perTiempoPosicionActual[rs_index-1]);
    AddLog(LOG_LEVEL_DEBUG);

    // Corrijo
    if(perTiempoPosicionActual[rs_index-1] > (per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1])){
      perTiempoPosicionActual[rs_index-1] = per_tiempo_total_recorrido[rs_index-1] + per_tiempo_adicional_lamas[rs_index-1];

      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: Corrijo al max recorrido mas lamas:%d"), rs_index, perTiempoPosicionActual[rs_index-1]);
      AddLog(LOG_LEVEL_DEBUG_MORE);
    }
  }
}

boolean RollershutterMqttData(void){
  char command [CMDSZ];
  char *type = NULL;
  uint16_t index;
  uint16_t index2;
  int16_t found = 0;
  uint16_t i = 0;

  // Extraigo el texto de la accion y el indice
  type = strrchr(XdrvMailbox.topic, '/');
  // Extraigo el texto de la accion y el indice
  index = 1;
  if (type != NULL) {
    type++;
    for (i = 0; i < strlen(type); i++){
      // type[i] = toupper(type[i]);
      type[i] = type[i];
    }
    while (isdigit(type[i-1])){
      i--;
    }
    if (i < strlen(type)){
      index = atoi(type +i);
    }
    type[i] = '\0';
  }

  snprintf_P(log_data, sizeof(log_data), PSTR("RS MQTT CMD: " D_INDEX " %d, " D_COMMAND " %s, " D_DATA " %s"),
    index, type, XdrvMailbox.data);
  AddLog(LOG_LEVEL_INFO);

  if (type != NULL) {
    int command_code = GetCommandCode(command, sizeof(command), type, kRollerShutterCommands);

    if ((CMND_RS_AC == command_code) && (index > 0) && (index <= PER_NUM_PERSIANAS)) {
      // Es un comando para la persiana y viene seguido de un indice
      snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: ======= Comando MQTT ======="), index);
      AddLog(LOG_LEVEL_INFO);

      // 0 Stop, 1 UP, 2 DOWN, 3 UPDATE_VALUES
      int valor = atoi(XdrvMailbox.data);
      switch(valor){
        case 0: // Stop
          RSStop(index);
          break;
        case 1: // Up
          RSUp(index);
          break;
        case 2: // Down
          RSDown(index);
          break;
        case 3: // Update info
          RSUpdateInfo(index);
          break;
      }
      // Pongo mqtt_data a cero para que no envie ningun mensaje adicional a mqtt
      mqtt_data[0] = '\0';
      found = 1;
    }else if ((CMND_RS_PC == command_code) && (index > 0) && (index <= PER_NUM_PERSIANAS)) {
      // Es un comando para la persiana y viene seguido de un indice
      int valor = atoi(XdrvMailbox.data);
      if ((valor >= 0) && (valor <= 100)) {
        // Viene un valor entre 0 y 100, correcto
        // Miro si viene un comando o el ultimo estado conocido para inicializar el porcentajeActual
        if(strstr(XdrvMailbox.topic, D_STAT) != NULL){
          // Es la inicializacion con el ultimo porcentaje enviado con retained
          snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: ======= Valor inicial retained: %d"), index, valor);
          AddLog(LOG_LEVEL_INFO);

          rsInitMqttValue(index, valor);
        }else{
          snprintf_P(log_data, sizeof(log_data), PSTR("RS%d: ======= Porcentaje pedido: %d"), index, valor);
          AddLog(LOG_LEVEL_INFO);

          RSPercent(index, valor);
        }
        found = 1;
      }
      // Pongo mqtt_data a cero para que no envie ningun mensaje adicional a mqtt
      mqtt_data[0] = '\0';
    }
  }
  return found;
}

/* struct XDRVMAILBOX { */
/*   uint16_t      valid; */
/*   uint16_t      index; */
/*   uint16_t      data_len; */
/*   int16_t       payload; */
/*   char         *topic; */
/*   char         *data; */
/* } XdrvMailbox; */
boolean RollerShutterCommand(void){
  char command [CMDSZ];
  boolean serviced = true;
  uint8_t ua_prefix_len = strlen(D_CMND_RS); // Length of the prefix RollerShutter

  snprintf_P(log_data, sizeof(log_data), "Command Action called: "
    "index: %d data_len: %d payload: %d topic: %s data: %s\n",
    XdrvMailbox.index,
    XdrvMailbox.data_len,
    XdrvMailbox.payload,
    (XdrvMailbox.payload >= 0 ? XdrvMailbox.topic : ""),
    (XdrvMailbox.data_len >= 0 ? XdrvMailbox.data : ""));

    AddLog(LOG_LEVEL_INFO);

  if(0 == strncasecmp_P(XdrvMailbox.topic, PSTR(D_CMND_RS), ua_prefix_len)){
    // command starts with RollerShutter
    int command_code = GetCommandCode(command, sizeof(command), XdrvMailbox.topic + ua_prefix_len, kRollerShutterCommands);
    if(CMND_RS_AC == command_code){
      snprintf_P(log_data, sizeof(log_data), "Command Action called: "
        "index: %d data_len: %d payload: %d topic: %s data: %s\n",
	      XdrvMailbox.index,
	      XdrvMailbox.data_len,
	      XdrvMailbox.payload,
	      (XdrvMailbox.payload >= 0 ? XdrvMailbox.topic : ""),
	      (XdrvMailbox.data_len >= 0 ? XdrvMailbox.data : ""));

        AddLog(LOG_LEVEL_INFO);
    }else if(CMND_RS_PC == command_code){
      snprintf_P(log_data, sizeof(log_data), "Command Percent called: "
        "index: %d data_len: %d payload: %d topic: %s data: %s\n",
	      XdrvMailbox.index,
	      XdrvMailbox.data_len,
	      XdrvMailbox.payload,
	      (XdrvMailbox.payload >= 0 ? XdrvMailbox.topic : ""),
	      (XdrvMailbox.data_len >= 0 ? XdrvMailbox.data : ""));

        AddLog(LOG_LEVEL_INFO);
    }else{
      serviced = false;
    }
  }else{
    serviced = false;
  }
  return serviced;
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

boolean Xdrv91(byte function){
  boolean result = false;

  switch (function) {
    case FUNC_INIT:
      RollerShutterInit();
      break;
    case FUNC_EVERY_50_MSECOND:
      RollerShutterHandler();
      break;
    case FUNC_EVERY_SECOND:
      // Mandar porcentaje actualizado
      break;
    case FUNC_COMMAND:
      snprintf_P(log_data, sizeof(log_data), "RollerShutter FUNC_COMMAND");
      AddLog(LOG_LEVEL_INFO);

      result = RollerShutterCommand();
      break;
    case FUNC_SHOW_SENSOR:
      snprintf_P(log_data, sizeof(log_data), "RollerShutter FUNC_SHOW_SENSOR");
      AddLog(LOG_LEVEL_INFO);

      break;
    case FUNC_SET_POWER:
      snprintf_P(log_data, sizeof(log_data), "RollerShutter FUNC_SET_POWER");
      AddLog(LOG_LEVEL_INFO);

      break;
    case FUNC_MQTT_SUBSCRIBE:
      //RollerShutterMqttSubscribe();
      break;
    case FUNC_MQTT_DATA:
      result = RollershutterMqttData();
      break;
    case FUNC_MQTT_INIT:
      snprintf_P(log_data, sizeof(log_data), "RollerShutter FUNC_MQTT_INIT");
      AddLog(LOG_LEVEL_INFO);
      RollerShutterMqttSubscribe();
      //RollershutterMqttData();
      break;
  }
  return result;
}

#endif
