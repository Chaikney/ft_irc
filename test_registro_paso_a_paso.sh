#!/bin/bash

# Test paso a paso del registro
PORT=6669
PASS="testpass123"

echo "=== TEST REGISTRO PASO A PASO ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

echo "🧪 Paso 1: Enviando PASS"
printf "PASS %s\n" "$PASS" >&3
sleep 0.5

echo "📥 Respuesta PASS:"
if read -t 2 -r line <&3; then
    echo "  $line"
else
    echo "  No hay respuesta"
fi

echo "🧪 Paso 2: Enviando NICK"
printf "NICK testuser\n" >&3
sleep 0.5

echo "📥 Respuesta NICK:"
if read -t 2 -r line <&3; then
    echo "  $line"
else
    echo "  No hay respuesta"
fi

echo "🧪 Paso 3: Enviando USER"
printf "USER testuser 0 * :Test User\n" >&3
sleep 0.5

echo "📥 Respuesta USER:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "001"; then
        echo "✅ Usuario registrado correctamente"
    else
        echo "❌ Usuario no registrado"
    fi
else
    echo "  No hay respuesta"
fi

echo "🧪 Paso 4: Enviando PING para verificar"
printf "PING :test\n" >&3
sleep 0.5

echo "📥 Respuesta PING:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "PONG"; then
        echo "✅ PING funciona correctamente"
    else
        echo "❌ PING no funciona"
    fi
else
    echo "  No hay respuesta"
fi

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== TEST COMPLETADO ==="
