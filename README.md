![](/files/mkr1000-aws-iot.png)
Con este proyecto podrás conectarte de forma segura a la nube de AWS IoT utilizando el protocolo MQTT. Por medio de la placa Arduino MKR100 y el sensor DHT22, lograras mandar mensajes desde y hacia la plataforma. 

## Antes de comenzar 
- Debes contar con una cuenta AWS, si aún no la tienes, crea una [aquí](https://aws.amazon.com/es/free/?all-free-tier.sort-by=item.additionalFields.SortRank&all-free-tier.sort-order=asc). AWS te ofrecerá una capa gratuita, puedes alojar el proyecto sin ningún costo por un año. 

- Crea un usuario IAM, sigue estas [instrucciones](https://docs.aws.amazon.com/es_es/rekognition/latest/dg/setting-up.html).

- Instala [Arduino](https://www.arduino.cc/en/software). Debes contar con la placa Arduino MKR100, para descargarla dirígete al IDE en *Herramientas > Placa > Gestor de tarjeta* y elige *Arduino SAMD*. Adicionalmente debes instalar las siguientes librerías: 
    - ArduinoBearSSL
    - ArduinoECCX08
    - ArduinoMqttClient
    - Arduino Cloud Provider Examples
    - WiFi101
    - LiquidCrystal_I2C
    - ArduinoJson
    - DHT

AWS IoT Core requiere que los dispositivos que se conectan mediante el protocolo MQTT usen certificados X.509 para la autenticación. Usaremos un sketch de ejemplo para generar una Solicitud de firma de certificado (CSR) en la placa y luego cargaremos esta CSR en la consola de AWS para crear un certificado X.509.

- Abre el IDE de Arduino y ve a *Archivo > Ejemplos -> ArduinoECCX08 -> Tools -> ECCX08CSR* y carga el sketch a la placa, luego, abre el Monitor serie.

Este esquema te pedirá que configures permanentemente su elemento criptográfico ATECC508A a ECC608A si no está configurado y bloqueado. NOTA: Este proceso de bloqueo es permanente e irreversible, pero es necesario para usar el elemento criptográfico: la configuración que establece el sketch permite usar 5 ranuras de clave privada con cualquier proveedor (o servidor) de la nube y se puede regenerar un CSR en cualquier momento para cada de las otras cuatro ranuras. Ahora, te pedirá información para incluir en el CSR, la mayoría de las entradas se pueden dejar en blanco, excepto el *Common Name*, en este proyecto se ingresó "MKR1000". Para este tutorial, usa la ranura 0 para generar y almacenar la clave privada utilizada para firmar el CSR (las ranuras 1 a 4 se pueden usar para generar y almacenar claves privadas adicionales si es necesario).

Copia el texto CSR generado incluidas las cadenas```"-----BEGIN CERTIFICATE REQUEST-----"``` y ```"-----END CERTIFICATE REQUEST-----"```. Ahora, guárdala en un nuevo archivo .txt. Este archivo se cargará a continuación en la consola de AWS. Observa la siguiente imagen y asegúrate de obtener el resultado con "Here's your CSR, enjoy!"

![](/files/eccx08.png)

Ahora que tenemos una CSR para identificar la placa, debes iniciar sesión en la consola de AWS y crear un certificado para ella.

- Busca AWS IoT Core y selecciónalo (nota: no todas las regiones admiten AWS IoT).

- Si es la primera vez que usas IoT Core, aparecerá una página de bienvenida, haz clic en *Comenzar* para continuar.

- Para registrar un objeto dirígete a *Administración -> Objetos -> Crear objeto* y selecciona *Crear un solo objeto*.

- Asigna un nombre al objeto, en este caso es “MKR1000” Las otras entradas del formulario se pueden dejar vacías.

- Cuando hayas dado nombre al objeto, lo siguiente será añadir un certificado. Como ya creaste una CSR en la placa, haz clic en el botón *Crear con CSR* y selecciona la CSR que se guardó en el archivo de texto anteriormente. Luego haz clic en *Cargar* y al finalizar haz clic en *Registrar Objeto*.

- Ahora necesitamos crear y adjuntar una política al certificado del objeto. Haz clic en *Seguridad -> Políticas -> Crear Política*. Crearas una política genérica, más adelante te sugerimos que crees una política más estricta. Crea una política con la siguiente configuración:

    ```
    Nombre: PermitirTodo
    Acción: iot: *
    ARN de recurso: *
    Efecto: Permitir
    ```
-  Creada la política, descarga el certificado. Dirígete a la sección de *Seguridad -> Certificados* y en el menú desplegable da clic en *Descargar*.

- Adjunta la política al certificado. Desde el menú desplegable da clic en *Asociar Política* y selecciona *PermitirTodo*. Finaliza dando clic en *Asociar*.

- Obtén el punto de enlace. Ve a configuración y copia y pega el *Punto de enlace*.


## Iniciando
- Clona el repositorio
    $ git clone https://github.com/erikvquiterio/Telemetria-con-Arduino-y-AWS-IoT-Core

- Reemplaza los valores de la pestaña *secrets* por los valores correspondientes a tus credenciales.

- Si todo es correcto, deberás obtener resultados similares a la siguiente imagen:

![](/files/test-send-data.png)

- Para ver los mensajes MQTT en la consola de IoT de AWS, dirígete a *Pruebas -> Suscribirse a un tema* y en *Tema de suscripción* escribe “outTopic” y da clic en *Suscribirse al tema* Aquí podrás ver todos los mensajes publicados. Recuerda que la latencia entre mensaje y mensaje es de 10 segundos, puedes modificar este valor en el sketch del programa.

![](/files/test-publish-data.png)

- Para mandar mensajes y visualizarlos en el display, debes cambiar a la pestaña *Publicar en un tema* y escribir “inTopic”. Modifica el mensaje que aparece justo abajo por el mensaje que quieras mandar y da clic en *Publicar en tema* 

![](/files/test-receive-data.png)
