#!/bin/bash

# Test de manejo robusto de canales
PORT=6668
PASS="testpass123"

echo "=== TEST DE MANEJO ROBUSTO DE CANALES ==="

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
printf "NICK channeluser\n" >&3
sleep 0.5
printf "USER channeluser 0 * :Channel User\n" >&3
sleep 1

# Limpiar respuestas de registro
while read -t 0.1 -r line <&3; do
    echo "Registro: $line"
done

echo "=== TEST 1: CREACIÓN Y GESTIÓN DE CANALES ==="

# Test JOIN canal válido
test_command "JOIN canal válido" "JOIN #validchannel" "JOIN" 3

# Test JOIN canal con nombre muy largo
test_command "JOIN canal nombre muy largo" "JOIN #$(printf 'a%.0s' {1..200})" "476" 3

# Test JOIN canal con caracteres especiales
test_command "JOIN canal con espacios" "JOIN #bad channel" "476" 3

# Test JOIN canal con comas
test_command "JOIN canal con comas" "JOIN #bad,channel" "476" 3

# Test JOIN canal sin #
test_command "JOIN canal sin #" "JOIN badchannel" "476" 3

# Test JOIN canal con doble #
test_command "JOIN canal con doble #" "JOIN ##doublechannel" "476" 3

# Test JOIN canal vacío
test_command "JOIN canal vacío" "JOIN #" "476" 3

echo "=== TEST 2: MÚLTIPLES CANALES ==="

# Test JOIN múltiples canales
test_command "JOIN múltiples canales" "JOIN #channel1,#channel2" "JOIN" 3

# Test PART múltiples canales
test_command "PART múltiples canales" "PART #channel1,#channel2" "PART" 3

echo "=== TEST 3: TOPIC Y GESTIÓN ==="

# Test TOPIC consultar canal existente
test_command "TOPIC consultar canal existente" "TOPIC #validchannel" "331" 3

# Test TOPIC establecer
test_command "TOPIC establecer" "TOPIC #validchannel :Nuevo tema del canal" "TOPIC" 3

# Test TOPIC consultar después de establecer
test_command "TOPIC consultar después de establecer" "TOPIC #validchannel" "332" 3

# Test TOPIC canal inexistente
test_command "TOPIC canal inexistente" "TOPIC #nonexistent" "403" 3

echo "=== TEST 4: NAMES Y LIST ==="

# Test NAMES canal existente
test_command "NAMES canal existente" "NAMES #validchannel" "353" 3

# Test NAMES canal inexistente
test_command "NAMES canal inexistente" "NAMES #nonexistent" "366" 3

# Test NAMES sin canal
test_command "NAMES sin canal" "NAMES" "366" 3

# Test LIST
test_command "LIST todos los canales" "LIST" "321" 3

# Test LIST canal específico
test_command "LIST canal específico" "LIST #validchannel" "322" 3

echo "=== TEST 5: PRIVMSG A CANALES ==="

# Test PRIVMSG a canal donde estás
test_command "PRIVMSG a canal donde estás" "PRIVMSG #validchannel :Hola desde el canal" "PRIVMSG" 3

# Test PRIVMSG a canal donde no estás
test_command "PRIVMSG a canal donde no estás" "PRIVMSG #otherchannel :Hola" "404" 3

# Test PRIVMSG a canal inexistente
test_command "PRIVMSG a canal inexistente" "PRIVMSG #nonexistent :Hola" "403" 3

# Test PRIVMSG mensaje muy largo
test_command "PRIVMSG mensaje muy largo" "PRIVMSG #validchannel :$(printf 'a%.0s' {1..1000})" "PRIVMSG" 3

echo "=== TEST 6: PART Y LIMPIEZA ==="

# Test PART canal donde estás
test_command "PART canal donde estás" "PART #validchannel" "PART" 3

# Test PART canal donde no estás
test_command "PART canal donde no estás" "PART #otherchannel" "442" 3

# Test PART canal inexistente
test_command "PART canal inexistente" "PART #nonexistent" "403" 3

# Test PART sin argumentos
test_command "PART sin argumentos" "PART" "461" 3

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
