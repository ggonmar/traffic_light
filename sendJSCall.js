function sendJSCall(mode) {
    modes = [{
        mode: 'green',
        bg: 'green',
        title: 'Abierto',
        message: 'El acceso esta permitido',
        button: 'Cerrar semaforo',
        disablebutton: false,
        onoffButton: 'Apagar',
        light: 'lime'
    }, {
        mode: 'amber',
        bg: 'orange',
        title: 'Procesando...',
        message: '...',
        button: 'Procesando...',
        disablebutton: true,
        onoffButton: 'Apagar',
        light: 'gold'
    }, {
        mode: 'red',
        bg: 'tomato',
        title: 'Cerrado',
        message: 'El acceso esta restringido',
        button: 'Abrir semaforo',
        disablebutton: false,
        onoffButton: 'Apagar',
        light: 'red'
    }, {
        mode: 'off',
        bg: '#73828a',
        title: 'OFF',
        message: 'El semaforo esta apagado',
        button: 'Desactivado',
        disablebutton: true,
        onoffButton: 'Encender',
        light: ''
    }];
    console.log(document.readyState);
    mode = modes.filter(e => e.mode == mode)[0];
    document.title = '[' + mode.title + '] Semaforo de Reunion 2.0';
    document.body.style.backgroundColor = mode.bg;
    document.getElementById('title').innerHTML = mode.title;
    document.getElementById('message').innerHTML = mode.message;
    document.getElementById('status-button').disabled = mode.disablebutton;
    document.getElementById('status-button').value = mode.button;
    modes.map(elems => elems.mode).filter(e => e != 'off').forEach(light => {
        document.querySelector('#' + light).style.fill = light === mode.mode ? mode.light : 'white'
    });
     document.getElementById('on-off-button').value = mode.onoffButton;
    console.log(mode);
}