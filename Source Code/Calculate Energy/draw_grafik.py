import matplotlib.pyplot as plt
import pandas as pd

# Membaca file CSV
df = pd.read_csv('huffman_sender_2ms.csv', header=None, names=['Waktu', 'Power'])

# Abaikan kolom Waktu dan buat waktu dimulai dari 0 dengan interval 1 ms
df['Waktu'] = range(0, len(df))  # waktu dimulai dari 0, interval 1 ms

# Plotting data
fig, ax = plt.subplots(figsize=(10, 6))
line, = ax.plot(df['Waktu'], df['Power'], marker='o', color='b', label='Power', linestyle='-')  # Marker 'o' untuk titik

# Menambahkan judul dan label sumbu
plt.title('Grafik Power per 1 ms', fontsize=16)
plt.xlabel('Waktu (ms)', fontsize=12)
plt.ylabel('Power', fontsize=12)
plt.grid(True)

# Menampilkan legenda
plt.legend()

# Menambahkan anotasi kosong
annot = ax.annotate("", xy=(0, 0), xytext=(20, 20),
                    textcoords="offset points", bbox=dict(boxstyle="round", fc="w"),
                    arrowprops=dict(arrowstyle="->"))
annot.set_visible(False)

# Fungsi untuk memperbarui anotasi ketika kursor mendekati titik
def update_annot(ind):
    x, y = line.get_data()
    annot.xy = (x[ind["ind"][0]], y[ind["ind"][0]])
    text = f"Waktu: {x[ind['ind'][0]]} ms\nPower: {y[ind['ind'][0]]}"
    annot.set_text(text)
    annot.get_bbox_patch().set_alpha(0.8)

# Fungsi untuk memeriksa apakah kursor berada di dekat titik
def hover(event):
    vis = annot.get_visible()
    if event.inaxes == ax:
        cont, ind = line.contains(event)
        if cont:
            update_annot(ind)
            annot.set_visible(True)
            fig.canvas.draw_idle()
        else:
            if vis:
                annot.set_visible(False)
                fig.canvas.draw_idle()

# Menghubungkan fungsi hover dengan event pergerakan mouse
fig.canvas.mpl_connect("motion_notify_event", hover)

# Menampilkan grafik
plt.show()
