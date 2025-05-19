#!/bin/bash
gcc monitor.c comun.c -o monitor -pthread
gcc banco.c comun.c -o banco -pthread
gcc usuario.c comun.c -o usuario -pthread

#########################################
# Script para:
# - Crear usuario nuevo
# - Copiar programa
# - Crear acceso directo en escritorio
# - Permitir ejecutar el programa con sudo sin contraseña
#########################################


# ✅ Verificar permisos
if [[ "$EUID" -ne 0 ]]; then
  echo "❌ Este script debe ejecutarse como root. Usa: sudo $0"
  exit 1
fi

# === CONFIGURACIÓN: ===
NUEVO_USUARIO="UsuarioBanco"
CLAVE="banco"
RUTA_ORIG="./"
RUTA_SCRIPT="arrancar_banco.sh"
NOMBRE_PROGRAMA="banco"


# Rutas
RUTA_DEST="/home/$NUEVO_USUARIO/SistemaBanco"
DESKTOP_DIR="/home/$NUEVO_USUARIO/Desktop"
LAUNCHER="$DESKTOP_DIR/$NOMBRE_PROGRAMA.desktop"
SCRIPT_COMPLETO="$RUTA_DEST/$RUTA_SCRIPT"

#Crear nuevo usuario
sudo useradd -m "$NUEVO_USUARIO"
echo "$NUEVO_USUARIO:$CLAVE" | sudo chpasswd

#Copiar programa
sudo cp -r "$RUTA_ORIG" "$RUTA_DEST"
sudo chown -R "$NUEVO_USUARIO:$NUEVO_USUARIO" "$RUTA_DEST"
sudo chmod -R u+rwX "$RUTA_DEST"

#Crear acceso directo en el escritorio
sudo mkdir -p "$DESKTOP_DIR"
sudo bash -c "cat > '$LAUNCHER'" <<EOF
[Desktop Entry]
Type=Application
Name=$NOMBRE_PROGRAMA
Exec=gnome-terminal -- bash -c 'cd /home/$NUEVO_USUARIO/SistemaBanco && ./banco ; exec bash'
Icon=utilities-terminal
Terminal=false
EOF

sudo chmod +x "$LAUNCHER"
sudo chown "$NUEVO_USUARIO:$NUEVO_USUARIO" "$LAUNCHER"

#Agregamos una regla sudo para poder ejecutar el script sin contraseña
SUDOERS_LINE="$NUEVO_USUARIO ALL=(ALL) NOPASSWD: $SCRIPT_COMPLETO"
echo "$SUDOERS_LINE" | sudo tee "/etc/sudoers.d/$NUEVO_USUARIO-programa" > /dev/null
sudo chmod 440 "/etc/sudoers.d/$NUEVO_USUARIO-programa"

# 5. INSTRUCCIONES PARA PODER EJECUTAR BANCO DESDE OTRO USUARIO
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                  ✅  CONFIGURACIÓN COMPLETA                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "📌 Usuario creado:           $NUEVO_USUARIO"
echo "🔑 Contraseña:               $CLAVE"
echo "📁 Programa copiado a:       $RUTA_DEST"
echo "📎 Acceso directo en:        $LAUNCHER"
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    📋 PASOS FINALES A SEGUIR               ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "1️⃣  Cierra tu sesión actual (haz log out)."
echo "2️⃣  En la pantalla de inicio de sesión, selecciona: '$NUEVO_USUARIO'"
echo "3️⃣  Ingresa la contraseña proporcionada."
echo "4️⃣  En el escritorio, haz doble clic en '$NOMBRE_PROGRAMA'."
echo "    ➤ Esto abrirá GNOME Terminal y ejecutará el programa con permisos."
echo ""
echo "✨ ¡Todo listo para comenzar a usar el sistema! ✨"
echo ""
