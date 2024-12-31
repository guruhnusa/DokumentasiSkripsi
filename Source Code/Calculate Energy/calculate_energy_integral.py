import numpy as np
import pandas as pd


# Fungsi untuk membaca data
def load_data(file_path):
    df = pd.read_csv(file_path, header=None, names=['time', 'power'], delimiter=',')
    df['time_sec'] = df['time'] / 1000  # Konversi waktu ke detik
    df['power_watt'] = df['power'] / 1000  # Konversi daya dari mW ke W
    return df

# Metode 1: Left Rectangle Rule
def left_rectangle_rule(df):
    delta_t = np.diff(df['time_sec'])  # Interval waktu antar data point
    energy = np.sum(df['power_watt'][:-1] * delta_t)  # Perkalian power dengan delta_t (kecuali poin terakhir)
    return energy

# Metode 2: Right Rectangle Rule
def right_rectangle_rule(df):
    delta_t = np.diff(df['time_sec'])
    energy = np.sum(df['power_watt'][1:] * delta_t)  # Perkalian power dengan delta_t (kecuali poin pertama)
    return energy

# Metode 3: Trapezoid Rule
def trapezoid_rule(df):
    energy = np.trapz(df['power_watt'], df['time_sec'])  # Menggunakan aturan trapezoidal
    return energy

# Main function untuk menjalankan semua metode dan menampilkan hasilnya
def main(file_path):
    # Baca data
    df = load_data(file_path)
    
    # Hitung energi menggunakan ketiga metode
    energy_left = left_rectangle_rule(df)
    energy_right = right_rectangle_rule(df)
    energy_trapezoid = trapezoid_rule(df)
    
    # Tampilkan hasil
    print("Total Energy (Left Rectangle Rule):", energy_left, "Joules")
    print("Total Energy (Right Rectangle Rule):", energy_right, "Joules")
    print("Total Energy (Trapezoid Rule):", energy_trapezoid, "Joules")

# Ganti path file dengan lokasi 'check_energy.csv'
main('check_energy.csv')
