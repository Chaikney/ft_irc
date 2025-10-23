#!/bin/bash

# Test de casos edge y límites
PORT=6668
PASS="testpass123"

echo "=== TEST DE CASOS EDGE Y LÍMITES ==="

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
printf "NICK edgeuser\n" >&3
sleep 0.5
printf "USER edgeuser 0 * :Edge User\n" >&3
sleep 1

# Limpiar respuestas de registro
while read -t 0.1 -r line <&3; do
    echo "Registro: $line"
done

echo "=== TEST 1: LÍMITES DE LONGITUD ==="

# Test NICK muy largo
test_command "NICK muy largo" "NICK $(printf 'a%.0s' {1..50})" "432" 3

# Test canal muy largo
test_command "Canal muy largo" "JOIN #$(printf 'a%.0s' {1..200})" "476" 3

# Test mensaje muy largo
test_command "Mensaje muy largo" "PRIVMSG #test :$(printf 'a%.0s' {1..1000})" "PRIVMSG" 3

# Test línea muy larga
test_command "Línea muy larga" "$(printf 'a%.0s' {1..1000})" "421" 3

echo "=== TEST 2: CARACTERES ESPECIALES ==="

# Test caracteres Unicode
test_command "Caracteres Unicode" "PRIVMSG #test :Hola 世界 🌍" "PRIVMSG" 3

# Test caracteres de control
test_command "Caracteres de control" "PRIVMSG #test :\x00\x01\x02\x03" "PRIVMSG" 3

# Test caracteres especiales en nick
test_command "Nick con caracteres especiales" "NICK user@#$%" "432" 3

echo "=== TEST 3: CASOS VACÍOS Y NULOS ==="

# Test comando vacío
test_command "Comando vacío" "" "" 3

# Test comando con solo espacios
test_command "Comando solo espacios" "   " "421" 3

# Test mensaje vacío
test_command "Mensaje vacío" "PRIVMSG #test :" "412" 3

# Test mensaje solo espacios
test_command "Mensaje solo espacios" "PRIVMSG #test :   " "412" 3

echo "=== TEST 4: ARGUMENTOS EXTREMOS ==="

# Test USER con argumentos extremos
test_command "USER con argumentos extremos" "USER $(printf 'a%.0s' {1..100}) 0 * :$(printf 'b%.0s' {1..200})" "USER" 3

# Test TOPIC muy largo
test_command "TOPIC muy largo" "TOPIC #test :$(printf 'c%.0s' {1..500})" "TOPIC" 3

echo "=== TEST 5: COMANDOS MALFORMADOS ==="

# Test comando con caracteres raros
test_command "Comando con caracteres raros" "PRIVMSG\x00#test :hello" "421" 3

# Test comando con múltiples espacios
test_command "Comando con múltiples espacios" "  PRIVMSG   #test   :hello  " "PRIVMSG" 3

# Test comando con tabs
test_command "Comando con tabs" "PRIVMSG\t#test\t:hello" "PRIVMSG" 3

echo "=== TEST 6: LÍMITES DE BUFFER ==="

# Test múltiples comandos en una línea
test_command "Múltiples comandos en línea" "PING :test\nJOIN #test\nPRIVMSG #test :hello" "PONG" 3

# Test comando sin terminador
test_command "Comando sin terminador" "PRIVMSG #test :hello" "PRIVMSG" 3

echo "=== TEST 7: CASOS DE TIMING ==="

# Test comandos muy rápidos
echo "🧪 Comandos muy rápidos"
for i in {1..10}; do
    printf "PRIVMSG #test :Mensaje $i\n" >&3
done
sleep 1

# Leer respuestas
echo "📥 Respuestas comandos rápidos:"
for i in {1..5}; do
    if read -t 0.1 -r line <&3; then
        echo "  $i: $line"
    else
        break
    fi
done
echo "✅ Comandos rápidos completados"
echo ""

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
