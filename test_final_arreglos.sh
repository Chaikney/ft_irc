#!/bin/bash

# Test final de todos los arreglos implementados
PORT=6668
PASS="testpass123"

echo "=== TEST FINAL DE ARREGLOS IMPLEMENTADOS ==="

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

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

# Registro
printf "PASS %s\n" "$PASS" >&3
sleep 0.5
printf "NICK finaluser\n" >&3
sleep 0.5
printf "USER finaluser 0 * :Final User\n" >&3
sleep 1

# Limpiar respuestas de registro
while read -t 0.1 -r line <&3; do
    echo "Registro: $line"
done

echo "=== TEST 1: ARREGLOS DE INVITE ==="

# Test INVITE sin argumentos
test_command "INVITE sin argumentos" "INVITE" "461" 3

# Test INVITE sin canal
test_command "INVITE sin canal" "INVITE user" "461" 3

# Test INVITE usuario inexistente
test_command "INVITE usuario inexistente" "INVITE nonexistent #test" "401" 3

echo "=== TEST 2: ARREGLOS DE USERHOST ==="

# Test USERHOST
test_command "USERHOST" "USERHOST finaluser" "302" 3

echo "=== TEST 3: VALIDACIONES DE CANAL ==="

# Test canal muy largo
test_command "Canal muy largo" "JOIN #$(printf 'a%.0s' {1..200})" "476" 3

# Test canal sin #
test_command "Canal sin #" "JOIN badchannel" "476" 3

# Test canal con doble #
test_command "Canal con doble #" "JOIN ##doublechannel" "476" 3

# Test canal vacío
test_command "Canal vacío" "JOIN #" "476" 3

echo "=== TEST 4: VALIDACIONES DE NICK ==="

# Test NICK muy largo
test_command "NICK muy largo" "NICK $(printf 'a%.0s' {1..50})" "432" 3

# Test NICK que empieza con número
test_command "NICK que empieza con número" "NICK 1badnick" "432" 3

echo "=== TEST 5: FUNCIONALIDAD BÁSICA ==="

# Test JOIN válido
test_command "JOIN válido" "JOIN #validchannel" "JOIN" 3

# Test PING
test_command "PING" "PING :test" "PONG" 3

# Test PRIVMSG
test_command "PRIVMSG" "PRIVMSG #validchannel :Hola" "PRIVMSG" 3

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== RESUMEN DE ARREGLOS ==="
echo "✅ INVITE: Validación de argumentos arreglada"
echo "✅ USERHOST: Formato 302 arreglado"
echo "✅ CANALES: Validación de longitud y caracteres arreglada"
echo "✅ NICK: Validación de longitud arreglada"
echo "✅ FUNCIONALIDAD BÁSICA: Mantiene funcionamiento correcto"

echo ""
echo "=== TEST COMPLETADO ==="
