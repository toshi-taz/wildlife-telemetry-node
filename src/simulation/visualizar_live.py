import pandas as pd
import folium
import os

INPUT_FILE = "data/telemetry_log.csv"
OUTPUT_MAP = "data/mapa_live.html"

def generar_mapa():
    if not os.path.exists(INPUT_FILE):
        print("No hay datos aún. Ejecuta serial_simulator.py primero.")
        return

    df = pd.read_csv(INPUT_FILE)
    print(f"Registros cargados: {len(df)}")

    centro_lat = df["latitude"].mean()
    centro_lon = df["longitude"].mean()

    mapa = folium.Map(location=[centro_lat, centro_lon], zoom_start=13, tiles="CartoDB positron")

    # Trayectoria
    puntos = list(zip(df["latitude"], df["longitude"]))
    folium.PolyLine(puntos, color="red", weight=2, opacity=0.8).add_to(mapa)

    # Puntos con info
    for _, row in df.iterrows():
        folium.CircleMarker(
            location=[row["latitude"], row["longitude"]],
            radius=4,
            color="red",
            fill=True,
            tooltip=f"{row['timestamp']} | {row['speed']} km/h | {row['satellites']} sats"
        ).add_to(mapa)

    # Inicio y fin
    folium.Marker(puntos[0], tooltip="Inicio", icon=folium.Icon(color="green")).add_to(mapa)
    folium.Marker(puntos[-1], tooltip="Último punto", icon=folium.Icon(color="red")).add_to(mapa)

    mapa.save(OUTPUT_MAP)
    print(f"Mapa guardado en {OUTPUT_MAP}")

if __name__ == "__main__":
    generar_mapa()
