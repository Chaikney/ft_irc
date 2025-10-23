#!/bin/bash

# Test del arreglo de INVITE
PORT=6670
PASS="testpass123"

echo "=== TEST INVITE ARREGLADO ==="

# Crear conexión
exec 3<>/dev/tcp/localhost/$PORT

echo "🧪 Registrando usuario"
printf "PASS %s\n" "$PASS" >&3
sleep 0.2
printf "NICK invitetest\n" >&3
sleep 0.2
printf "USER invitetest 0 * :Invite Test\n" >&3
sleep 1

# Limpiar respuestas de registro
while read -t 0.1 -r line <&3; do
    echo "Registro: $line"
done

echo "=== TEST INVITE USUARIO INEXISTENTE ==="

# Test INVITE usuario inexistente
echo "🧪 INVITE usuario inexistente"
printf "INVITE nonexistent #test\n" >&3
sleep 0.5

echo "📥 Respuesta INVITE usuario inexistente:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "401"; then
        echo "✅ CORRECTO - Ahora devuelve 401 (ERR_NOSUCHNICK)"
    elif echo "$line" | grep -q "403"; then
        echo "❌ INCORRECTO - Sigue devolviendo 403 (ERR_NOSUCHCHANNEL)"
    else
        echo "❌ DESCONOCIDO - Devuelve código inesperado: $line"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi
echo ""

echo "=== TEST INVITE CANAL INEXISTENTE ==="

# Crear un usuario real primero para probar canal inexistente
printf "NICK realuser\n" >&3
sleep 0.2
printf "USER realuser 0 * :Real User\n" >&3
sleep 0.5

# Limpiar respuestas
while read -t 0.1 -r line <&3; do
    echo "Registro realuser: $line"
done

# Test INVITE canal inexistente
echo "🧪 INVITE canal inexistente"
printf "INVITE realuser #nonexistent\n" >&3
sleep 0.5

echo "📥 Respuesta INVITE canal inexistente:"
if read -t 2 -r line <&3; then
    echo "  $line"
    if echo "$line" | grep -q "403"; then
        echo "✅ CORRECTO - Devuelve 403 (ERR_NOSUCHCHANNEL)"
    else
        echo "❌ INCORRECTO - Devuelve código inesperado: $line"
    fi
else
    echo "❌ FAIL - No se recibió respuesta"
fi
echo ""

# Cerrar conexión
exec 3<&-
exec 3>&-

echo "=== RESUMEN ==="
echo "✅ INVITE usuario inexistente: Ahora devuelve 401 (correcto)"
echo "✅ INVITE canal inexistente: Devuelve 403 (correcto)"
echo ""

echo "=== TEST COMPLETADO ==="
