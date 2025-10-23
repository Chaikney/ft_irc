#!/bin/bash

# Test de múltiples usuarios y conexiones
PORT=6668
PASS="testpass123"

echo "=== TEST DE MÚLTIPLES USUARIOS Y CONEXIONES ==="

# Función para registrar usuario
register_user() {
    local fd="$1"
    local nick="$2"
    
    printf "PASS %s\n" "$PASS" >&$fd
    sleep 0.2
    printf "NICK %s\n" "$nick" >&$fd
    sleep 0.2
    printf "USER %s 0 * :Test User\n" "$nick" >&$fd
    sleep 0.5
    
    # Limpiar respuestas de registro
    while read -t 0.1 -r line <&$fd; do
        echo "Registro $nick: $line"
    done
}

# Función para probar comando
test_command() {
    local test_name="$1"
    local command="$2"
    local expected="$3"
    local fd="$4"
    
    echo "🧪 $test_name"
    printf "%s\n" "$command" >&$fd
    sleep 0.5
    
    local response=""
    while read -t 1 -r line <&$fd; do
        response="$response$line\n"
    done
    
    if echo -e "$response" | grep -q "$expected"; then
        echo "✅ PASS - $test_name"
    else
        echo "❌ FAIL - $test_name"
        echo "   Esperado: $expected"
        echo "   Recibido: $response"
    fi
    echo ""
}

echo "=== TEST 1: MÚLTIPLES CONEXIONES SIMULTÁNEAS ==="

# Crear múltiples conexiones
exec 3<>/dev/tcp/localhost/$PORT
exec 4<>/dev/tcp/localhost/$PORT
exec 5<>/dev/tcp/localhost/$PORT

# Registrar usuarios
echo "Registrando usuario 1..."
register_user 3 "user1"

echo "Registrando usuario 2..."
register_user 4 "user2"

echo "Registrando usuario 3..."
register_user 5 "user3"

echo "=== TEST 2: NICK DUPLICADO ==="

# Test NICK duplicado
echo "🧪 NICK duplicado"
printf "NICK user1\n" >&4
sleep 0.5

response=""
while read -t 1 -r line <&4; do
    response="$response$line\n"
done

if echo -e "$response" | grep -q "433"; then
    echo "✅ PASS - NICK duplicado"
else
    echo "❌ FAIL - NICK duplicado"
    echo "   Recibido: $response"
fi
echo ""

echo "=== TEST 3: MÚLTIPLES USUARIOS EN CANAL ==="

# Todos se unen al mismo canal
test_command "Usuario 1 se une al canal" "JOIN #multichan" "JOIN" 3
test_command "Usuario 2 se une al canal" "JOIN #multichan" "JOIN" 4
test_command "Usuario 3 se une al canal" "JOIN #multichan" "JOIN" 5

echo "=== TEST 4: MENSAJES ENTRE USUARIOS ==="

# Usuario 1 envía mensaje al canal
test_command "Usuario 1 envía mensaje al canal" "PRIVMSG #multichan :Hola desde user1" "PRIVMSG" 3

# Usuario 2 envía mensaje al canal
test_command "Usuario 2 envía mensaje al canal" "PRIVMSG #multichan :Hola desde user2" "PRIVMSG" 4

# Usuario 1 envía mensaje privado a usuario 2
test_command "Usuario 1 envía mensaje privado a usuario 2" "PRIVMSG user2 :Mensaje privado" "PRIVMSG" 3

echo "=== TEST 5: NAMES CON MÚLTIPLES USUARIOS ==="

# Test NAMES con múltiples usuarios
test_command "NAMES con múltiples usuarios" "NAMES #multichan" "353" 3

echo "=== TEST 6: WHO CON MÚLTIPLES USUARIOS ==="

# Test WHO con múltiples usuarios
test_command "WHO canal con múltiples usuarios" "WHO #multichan" "352" 3

echo "=== TEST 7: PART CON MÚLTIPLES USUARIOS ==="

# Usuario 1 sale del canal
test_command "Usuario 1 sale del canal" "PART #multichan" "PART" 3

# Verificar que los otros usuarios siguen en el canal
test_command "NAMES después de PART" "NAMES #multichan" "353" 4

echo "=== TEST 8: QUIT CON MÚLTIPLES USUARIOS ==="

# Usuario 2 hace QUIT
test_command "Usuario 2 hace QUIT" "QUIT :Adiós" "QUIT" 4

# Verificar que usuario 3 sigue en el canal
test_command "NAMES después de QUIT" "NAMES #multichan" "353" 5

echo "=== TEST 9: CONEXIONES RÁPIDAS ==="

# Crear y cerrar conexiones rápidamente
for i in {1..3}; do
    echo "🧪 Conexión rápida $i"
    exec 6<>/dev/tcp/localhost/$PORT
    register_user 6 "quick$i"
    printf "JOIN #quicktest\n" >&6
    sleep 0.1
    exec 6<&-
    exec 6>&-
    echo "✅ Conexión rápida $i completada"
done

# Cerrar conexiones restantes
exec 3<&-
exec 3>&-
exec 5<&-
exec 5>&-

echo "=== TEST COMPLETADO ==="
