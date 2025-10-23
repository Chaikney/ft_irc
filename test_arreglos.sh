#!/bin/bash

# Test para verificar los arreglos realizados
PORT=6668
PASS="testpass123"

echo "=== TEST DE ARREGLOS ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

# Test 1: Registro completo
echo "🧪 Test 1: Registro completo"
printf "PASS %s\n" "$PASS" >&3
sleep 1
printf "NICK testuser\n" >&3
sleep 1
printf "USER testuser 0 * :Test User\n" >&3
sleep 1

echo "📥 Respuestas de registro:"
for i in {1..5}; do
    if read -t 1 -r line <&3; then
        echo "  $i: $line"
    else
        break
    fi
done

# Test 2: PASS después de registro (debería devolver 462)
echo ""
echo "🧪 Test 2: PASS después de registro"
printf "PASS wrongpass\n" >&3
sleep 1

echo "📥 Respuesta PASS después de registro:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "462"; then
        echo "✅ PASS - Código 462 correcto"
    else
        echo "❌ FAIL - Código incorrecto"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi

# Test 3: JOIN canal
echo ""
echo "🧪 Test 3: JOIN canal"
printf "JOIN #testchannel\n" >&3
sleep 1

echo "📥 Respuestas JOIN:"
for i in {1..3}; do
    if read -t 1 -r line <&3; then
        echo "  $i: $line"
    else
        break
    fi
done

# Test 4: PRIVMSG
echo ""
echo "🧪 Test 4: PRIVMSG"
printf "PRIVMSG #testchannel :Hola mundo\n" >&3
sleep 1

echo "📥 Respuesta PRIVMSG:"
if read -t 2 -r line <&3; then
    echo "  $line"
else
    echo "  Sin respuesta (normal en IRC)"
fi

# Test 5: PING
echo ""
echo "🧪 Test 5: PING"
printf "PING :test\n" >&3
sleep 1

echo "📥 Respuesta PING:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "PONG"; then
        echo "✅ PASS - PONG recibido"
    else
        echo "❌ FAIL - PONG no recibido"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi

# Cerrar conexión
exec 3<&-
exec 3>&-

echo ""
echo "=== TEST COMPLETADO ==="
