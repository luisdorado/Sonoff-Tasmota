#define PER_NUM_PERSIANAS 1

// Persiana 1
// Reles
#define PERSIANA_UP_ACTUATOR_1 1
#define PERSIANA_DOWN_ACTUATOR_1 2
// Tiempo total en ms que tarda en hacer todo el recorrido
#ifndef PER_TIEMPO_TOTAL_RECORRIDO_1
#define PER_TIEMPO_TOTAL_RECORRIDO_1 19000 // ms
#endif
// Tiempo para reajustes. Cuando se le pide subir del todo, se añade al tiempo del recorrido para asegurar que llega arriba del todo
#define PER_TIEMPO_AJUSTE_RECORRIDO_1 1000 // segs 2
// Tiempo adicional que tarda en bajar las lamas
#ifndef PER_TIEMPO_ADICIONAL_1
#define PER_TIEMPO_ADICIONAL_1 4000 // 2.6 segs
#endif

// Persiana 2
// Reles
#define PERSIANA_UP_ACTUATOR_2 3
#define PERSIANA_DOWN_ACTUATOR_2 4
// Tiempo total en ms que tarda en hacer todo el recorrido
#ifndef PER_TIEMPO_TOTAL_RECORRIDO_2
#define PER_TIEMPO_TOTAL_RECORRIDO_2 17000 // segs 22
#endif
// Tiempo para reajustes. Cuando se le pide subir del todo, se añade al tiempo del recorrido para asegurar que llega arriba del todo
#define PER_TIEMPO_AJUSTE_RECORRIDO_2 1000 // segs 2
// Tiempo adicional que tarda en bajar las lamas
#ifndef PER_TIEMPO_ADICIONAL_2
#define PER_TIEMPO_ADICIONAL_2 4220 // 2.6 segs
#endif

// Tiempo que debe pasar entre envios de estado
const long PER_TIEMPO_ENTRE_ENVIOS_ESTADO=3000;
// Tiempo que debe pasar entre envios de porcentaje
const long PER_TIEMPO_ENTRE_ENVIOS_PORCENTAJE=3000;

// NO EDITAR
// Internal representation of the cover state.
#define RS_IDLE 0
#define RS_UP 1
#define RS_DOWN 2
#define RS_NOTSEND 3

// ENVIOS DE INFO
// Guarda si se ha cambiado el estado del motor (y hay que notificarlo al GW)
bool cambiadoEstado[2] = {false,false};
// Guarda el ultimo estado del motor enviado
int perStateEnviado[2] = {RS_NOTSEND,RS_NOTSEND};
// Porcentaje de persiana enviado por ultima vez al GW
int perPorcentajeUltimoEnviado[2] = {255,255}; // 0 ==> Persiana subida, 100 => Persiana bajada
// Si se ha mandado el estado alguna vez al GW
bool perInitial_state_sent[2] = {false,false};
// Momento en el que se han enviado datos al GW por ultima vez
unsigned long perUltimoEnvioEstado[2] = {0,0};
unsigned long perUltimoEnvioPorcentaje[2] = {0,0};

// Configuracion
int persiana_up_actuator[2] = {PERSIANA_UP_ACTUATOR_1,PERSIANA_UP_ACTUATOR_2};
int persiana_down_actuator[2] = {PERSIANA_DOWN_ACTUATOR_1,PERSIANA_DOWN_ACTUATOR_2};
// Tiempo total en ms que tarda en hacer todo el recorrido
int per_tiempo_total_recorrido[2] = {PER_TIEMPO_TOTAL_RECORRIDO_1,PER_TIEMPO_TOTAL_RECORRIDO_2};
// Tiempo para reajustes. Cuando se le pide subir del todo, se añade al tiempo del recorrido para asegurar que llega arriba del todo
int per_tiempo_ajuste_recorrido[2] = {PER_TIEMPO_AJUSTE_RECORRIDO_1,PER_TIEMPO_AJUSTE_RECORRIDO_2};
int per_tiempo_adicional_lamas[2] = {PER_TIEMPO_ADICIONAL_1,PER_TIEMPO_ADICIONAL_2};

// Estado actual del motor
int perState[2] = {RS_IDLE,RS_IDLE};
// Para contabilizar el tiempo en ms desde que inicio la subida o bajada del motor (y calcular el porcentaje en base al valor total del recorrido)
unsigned long perTiempoInicioMotor[2] = {0,0};
// Cuando comienzo a mover el motor guardo en esta variable el total de ms que deberían quedarle para llegar al porcentaje al que se le manda ir
unsigned long perTiempoAMoverMotor[2] = {0,0};
// Posicion en ms con respecto a arriba en la que inicio el movimiento
long perTiempoPosicionInicial[2] = {0,0};
// Posicion en ms con respecto a arriba en la que me encuentro
long perTiempoPosicionActual[2] = {0,0};
// Indica si se ha cargado el ultimo estado persistente de MQTT
bool inicializado[2] = {false,false};

int ciclosSerial = 0; // Permite mostrar el estado cada x ciclos en el serie
