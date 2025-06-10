function menubar() {
    const menu = document.getElementById('menuu');
    menu.classList.toggle('activo');
}

function mostrarPantallaCarga() {
    const pantallaCarga = document.getElementById('pantalla-carga');
    const loader = pantallaCarga.querySelector('.loader-bolas');
    pantallaCarga.style.display = 'flex';
    loader.classList.remove('stop-animation');
}

function ocultarPantallaCarga() {
    const pantallaCarga = document.getElementById('pantalla-carga');
    const loader = pantallaCarga.querySelector('.loader-bolas');
    pantallaCarga.style.display = 'none';
    loader.classList.add('stop-animation');
}

function showContent(seccion) {
    if (seccion === 'salir') {
        if (confirm("¿Desea realmente salir?")) {
            mostrarPantallaCarga();
            fetch('/reiniciar', { method: 'POST' })
                .then(() => setTimeout(() => {
                    ocultarPantallaCarga();
                    location.reload();
                }, 1000));
        }
        return;
    }

    mostrarPantallaCarga();

    const secciones = ['apartado-inicio', 'apartado-pruebas', 'apartado-parametros'];
    secciones.forEach(id => document.getElementById(id).style.display = 'none');
    document.getElementById('menuu').classList.remove('activo');

    setTimeout(() => {
        ocultarPantallaCarga();

        switch (seccion) {
            case 'inicio':
                document.getElementById('apartado-inicio').style.display = 'block';
                fetchAndDisplayParameters();
                break;
            case 'pruebas':
                document.getElementById('apartado-pruebas').style.display = 'block';
                break;
            case 'parametros':
                document.getElementById('apartado-parametros').style.display = 'block';
                break;
            default:
                console.warn(`Sección desconocida: ${seccion}`);
        }
    }, 1000);
}

function fetchAndDisplayParameters() {
    console.log("Solicitando parámetros actuales desde el servidor...");
    fetch("/get-parametros")
        .then(response => {
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            return response.json();
        })
        .then(data => {
            const resumenId = document.getElementById('resumen-id');
            const resumenZona = document.getElementById('resumen-zona');
            const resumenTipo = document.getElementById('resumen-tipo-sensor');
            

            const tipoSensorMap = {
                0: "0 - Gas LP",
                1: "1 - Humo",
                2: "2 - Movimiento",
                3: "3 - Puerta",
                4: "4 - Ventana",
                5: "5 - Cortina",
                6: "6 - Botón Físico/Pánico",
                7: "7 - Palanca"
            };

            const tipoSensorText = tipoSensorMap[data.tipo] || `Tipo desconocido (${data.tipo})`;

            if (resumenId && resumenZona && resumenTipo && resumenActualizacion && resumenSenal) {
                resumenId.textContent = data.id;
                resumenZona.textContent = data.zona;
                resumenTipo.textContent = tipoSensorText;
                resumenActualizacion.textContent = new Date().toLocaleString();

                let zonaFormateada = String(data.zona).padStart(3, '0');
                const senalConcatenada = `${String(data.id).padStart(4, '0')}${String(data.tipo)}${zonaFormateada}`;
                resumenSenal.textContent = senalConcatenada;

                console.log("Parámetros actualizados correctamente.");
            } else {
                console.error("No se encontraron todos los elementos de resumen.");
            }
        })
        .catch(error => {
            console.error("Error al obtener parámetros:", error);
            const resumenActualizacion = document.getElementById('resumen-actualizacion');
            if (resumenActualizacion) resumenActualizacion.textContent = "Error al cargar";
        });
}

function enviarLora(senalConcatenada) {
    fetch('/enviar-lora', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ senalConcatenada })
    })
    .then(response => response.json())
    .then(data => {
        if (data.status) {
            alert(`Mensaje "${senalConcatenada}" enviado correctamente por LoRa.`);
        } else if (data.error) {
            alert("Error: " + data.error);
        }
    })
    .catch(error => {
        alert("Error de comunicación con el servidor.");
        console.error(error);
    });
}

function mostrarPantallaLora(numero) {
    console.log("Solicitando mostrar pantalla en la tarjeta:", numero);
    fetch("/mostrar-pantalla", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ numero })
    })
    .then(response => response.json())
    .then(data => {
        if (data.status) {
            console.log("Pantalla mostrada correctamente:", data.status);
        } else if (data.error) {
            alert("Error al solicitar pantalla: " + data.error);
        } else {
            console.log("Respuesta desconocida al solicitar pantalla:", data);
        }
    })
    .catch(error => {
        alert("Error de comunicación con el servidor al solicitar pantalla.");
        console.error("Error en fetch:", error);
    });
}

document.addEventListener('DOMContentLoaded', () => {
    fetchAndDisplayParameters();

    const form = document.getElementById('form-parametros');
    if (form) {
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            const idAlarma = document.getElementById('id-alarma').value.trim();
            const zona = document.getElementById('zona').value.trim();
            const tipoSensorSelect = document.getElementById('tipo-sensor');
            const tipoSensor = tipoSensorSelect.value.trim();
            const tipoSensorText = tipoSensorSelect.options[tipoSensorSelect.selectedIndex].text;

            if (!/^\d{4}$/.test(idAlarma)) {
                alert('El ID de la alarma debe ser un número de 4 dígitos.');
                return;
            }

            if (!/^\d{1,3}$/.test(zona)) {
                alert('La zona debe ser un número de 1 a 3 dígitos.');
                return;
            }

            const parametros = {
                "id-alarma": idAlarma,
                "zona": zona,
                "tipo-sensor": tipoSensor
            };

            mostrarPantallaCarga();

            console.log("Enviando parámetros:", parametros);
            fetch("/guardar-parametros", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(parametros)
            })
            .then(response => response.json())
            .then(data => {
                ocultarPantallaCarga();
                if (data.status) {
                    alert(data.status);
                    document.getElementById('resumen-id').textContent = idAlarma;
                    document.getElementById('resumen-zona').textContent = zona;
                    document.getElementById('resumen-tipo-sensor').textContent = tipoSensorText;
                    document.getElementById('resumen-actualizacion').textContent = new Date().toLocaleString();

                    let zonaFormateada = zona.padStart(3, '0');
                    const senalConcatenada = `${idAlarma}${tipoSensor}${zonaFormateada}`;
                    document.getElementById('senal-registrada').textContent = senalConcatenada;

                    console.log("Resumen actualizado con señal:", senalConcatenada);
                } else if (data.error) {
                    alert("Error: " + data.error);
                } else {
                    alert("Respuesta desconocida del servidor.");
                }
            })
            .catch(error => {
                ocultarPantallaCarga();
                alert("Error de comunicación con el servidor.");
                console.error("Error en fetch:", error);
            });
        });
    }

    const btnEnviarRF = document.getElementById('btn-enviar-rf');
    if (btnEnviarRF) {
        btnEnviarRF.addEventListener('click', () => {
            console.log("Enviando señal RF de prueba...");
            fetch("/enviar-rf-prueba", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({})
            })
            .then(response => response.json())
            .then(data => {
                if (data.status) {
                    alert(data.status);
                } else if (data.error) {
                    alert("Error: " + data.error);
                } else {
                    alert("Respuesta desconocida del servidor.");
                }
            })
            .catch(error => {
                alert("Error de comunicación con el servidor al enviar RF.");
                console.error("Error en fetch:", error);
            });
        });
    }
});
