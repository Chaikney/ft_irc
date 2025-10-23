#!/bin/bash

# Test de comandos avanzados
PORT=6668
PASS="testpass123"

echo "=== TEST DE COMANDOS AVANZADOS ==="

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

# Crear conexión principal
exec 3<>/dev/tcp/localhost/$PORT

# Registro
printf "PASS %s\n" "$PASS" >&3
sleep 0.5
printf "NICK admin\n" >&3
sleep 0.5
printf "USER admin 0 * :Admin User\n" >&3
sleep 1

# Limpiar respuestas de registro
while read -t 0.1 -r line <&3; do
    echo "Registro: $line"
done

echo "=== TEST 1: COMANDOS MODE ==="

# Test MODE canal
test_command "MODE consultar canal" "MODE #testchannel" "324" 3

# Test MODE usuario
test_command "MODE consultar usuario" "MODE admin" "221" 3

# Test MODE sin argumentos
test_command "MODE sin argumentos" "MODE" "461" 3

# Test MODE canal inexistente
test_command "MODE canal inexistente" "MODE #nonexistent" "403" 3

echo "=== TEST 2: COMANDOS KICK ==="

# Crear canal primero
printf "JOIN #kicktest\n" >&3
sleep 0.5
while read -t 0.1 -r line <&3; do
    echo "JOIN: $line"
done

# Test KICK sin argumentos
test_command "KICK sin argumentos" "KICK" "461" 3

# Test KICK sin usuario
test_command "KICK sin usuario" "KICK #kicktest" "461" 3

# Test KICK usuario inexistente
test_command "KICK usuario inexistente" "KICK #kicktest nonexistent" "401" 3

# Test KICK canal inexistente
test_command "KICK canal inexistente" "KICK #nonexistent user" "403" 3

echo "=== TEST 3: COMANDOS INVITE ==="

# Test INVITE sin argumentos
test_command "INVITE sin argumentos" "INVITE" "461" 3

# Test INVITE sin canal
test_command "INVITE sin canal" "INVITE user" "461" 3

# Test INVITE usuario inexistente
test_command "INVITE usuario inexistente" "INVITE nonexistent #kicktest" "401" 3

# Test INVITE canal inexistente
test_command "INVITE canal inexistente" "INVITE user #nonexistent" "403" 3

echo "=== TEST 4: COMANDOS DE USUARIO AVANZADOS ==="

# Test WHO con canal
test_command "WHO canal" "WHO #kicktest" "352" 3

# Test WHO usuario
test_command "WHO usuario" "WHO admin" "352" 3

# Test WHOIS usuario
test_command "WHOIS usuario" "WHOIS admin" "311" 3

# Test WHOIS usuario inexistente
test_command "WHOIS usuario inexistente" "WHOIS nonexistent" "401" 3

# Test AWAY establecer
test_command "AWAY establecer" "AWAY :Estoy ausente" "306" 3

# Test AWAY quitar
test_command "AWAY quitar" "AWAY" "305" 3

# Test USERHOST
test_command "USERHOST" "USERHOST admin" "302" 3

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
