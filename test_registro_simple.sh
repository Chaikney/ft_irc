#!/bin/bash

# Test simple de registro
PORT=6668
PASS="testpass123"

echo "=== TEST REGISTRO SIMPLE ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

echo "🧪 Enviando PASS"
printf "PASS %s\n" "$PASS" >&3
sleep 0.5

echo "📥 Respuesta PASS:"
if read -t 2 -r line <&3; then
    echo "  $line"
else
    echo "  No hay respuesta"
fi

echo "🧪 Enviando NICK"
printf "NICK testuser\n" >&3
sleep 0.5

echo "📥 Respuesta NICK:"
if read -t 2 -r line <&3; then
    echo "  $line"
else
    echo "  No hay respuesta"
fi

echo "🧪 Enviando USER"
printf "USER testuser 0 * :Test User\n" >&3
sleep 0.5

echo "📥 Respuesta USER:"
if read -t 2 -r line <&3; then
    echo "  $line"
else
    echo "  No hay respuesta"
fi

echo "🧪 Enviando PING para verificar registro"
printf "PING :test\n" >&3
sleep 0.5

echo "📥 Respuesta PING:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "PONG"; then
        echo "✅ Usuario registrado correctamente"
    else
        echo "❌ Usuario no registrado"
    fi
else
    echo "  No hay respuesta"
fi

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
