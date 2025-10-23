#!/bin/bash

# Test específico de NICK
PORT=6668
PASS="testpass123"

echo "=== TEST ESPECÍFICO DE NICK ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

# Registro básico
printf "PASS %s\n" "$PASS" >&3
sleep 0.5
printf "NICK testuser\n" >&3
sleep 0.5
printf "USER testuser 0 * :Test User\n" >&3
sleep 1

# Limpiar respuestas de registro
while read -t 0.1 -r line <&3; do
    echo "Registro: $line"
done

echo "=== TEST NICK INVÁLIDO DESPUÉS DE REGISTRO ==="

# Test NICK inválido después de registro
echo "🧪 NICK inválido después de registro"
printf "NICK @invalid\n" >&3
sleep 0.5

echo "📥 Respuesta NICK inválido:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "432"; then
        echo "✅ PASS - NICK inválido devuelve 432"
    else
        echo "❌ FAIL - NICK inválido no devuelve 432"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi
echo ""

echo "=== TEST NICK VÁLIDO DESPUÉS DE REGISTRO ==="

# Test NICK válido después de registro
echo "🧪 NICK válido después de registro"
printf "NICK newvalidnick\n" >&3
sleep 0.5

echo "📥 Respuesta NICK válido:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "NICK"; then
        echo "✅ PASS - NICK válido funciona"
    else
        echo "❌ FAIL - NICK válido no funciona"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi
echo ""

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
