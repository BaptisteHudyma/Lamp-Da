import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import requests
import serial
import serial.tools.list_ports
import threading
import lampda_serial
import os

# used to translate text strings
import TextLangage as tx


# -------------------
# Onglet 1 : Mise à jour depuis GitHub
# -------------------
class TabOfficialUpdate(ttk.Frame):
    """
    permet de mettre a jour la lamp-da depuis le depot officiel sur git
    si la version de la lampe est trop vielle, il faudra choisir manuellement le type de la lampe
    """

    def __init__(self, parent):
        """
        :param parent: frame parent pour placer l'onglet
        """
        super().__init__(parent)
        ttk.Button(
            self,
            text=tx.get_text_translation(tx.FIND_CONNECTED_DRIVE_ID),
            command=self.search_lampda,
        ).pack(pady=20)
        # ttk.Button(self, text="Télécharger le firmware officiel", command=self.download_latest_firmware).pack(pady=20)

        # Zone où on va afficher les boutons des ports détectés
        self.lampda_container = tk.Frame(self)
        self.lampda_container.pack(fill="both", expand=True, pady=10)

        self.all_release = lampda_serial.get_releases()
        self.latest_release = lampda_serial.get_most_recent_version(self.all_release)

        self.manual_lamp_type = None
        self.list_lampda = None

    def search_lampda(self, only_update=False):
        """
        cherche toutes les lamp-da connecté a l'ordinateur
        :param only_update: mettant a jour uniquement les lamp-da deja trouvé

        """
        # Nettoyer les anciens boutons
        for widget in self.lampda_container.winfo_children():
            widget.pack_forget()
            widget.destroy()

        if only_update is False:
            self.list_lampda = lampda_serial.find_lampda()
        else:
            self.list_lampda = lampda_serial.find_lampda(
                [com for com, lampda in self.list_lampda]
            )
        if not self.list_lampda:
            label = tk.Label(
                self.lampda_container, text=tx.get_text_translation(tx.NO_LAMP_DETECTED_ID), fg="red"
            )
            label.pack()
            return

        # Création d'un bouton pour chaque port détecté
        for port, lampda in self.list_lampda:
            if lampda.type == "unflashed":
                label_text=tx.get_text_translation(tx.LAMP_NO_PROGRAM_ID)%port
                etat = tk.NORMAL
            elif lampda.user_v == self.latest_release.tag:
                label_text = tx.get_text_translation(tx.LAMP_ALREADY_UPDATED_ID)%(lampda.type)
                etat = tk.DISABLED
            else:
                label_text = tx.get_text_translation(tx.LAMP_UPDATE_TARGET_ID)%(lampda.type, lampda.user_v, self.latest_release.tag)
                etat = tk.NORMAL

            port_button = ttk.Button(
                self.lampda_container,
                text=label_text,
                command=lambda p=port, t=lampda: self.flash_lampda(p, t),
                state=etat,
            )
            port_button.pack(pady=5, fill="x")

            if etat != tk.NORMAL:
                port_force_button = ttk.Button(
                    self.lampda_container,
                    text=tx.get_text_translation(tx.FORCE_UPDATE_ID),
                    command=lambda p=port, t=lampda: self.flash_lampda(p, t),
                    state=tk.NORMAL,
                )
                port_force_button.pack(padx=5, fill="x")


    def flash_lampda(self, port, lampda: lampda_serial.LampDa):
        """

        :param port: port de la lamp-da a mettre a jour
        :param lampda: information de la lamp-da a mettre a jour

        """
        print("on flash ", port, lampda)
        if lampda.type not in ["simple", "indexable"]:
            self.ask_user_manual(
                tx.get_text_translation(tx.WHAT_LAMP_TYPE_ID), ["simple", "indexable"]
            )
            lampda.type = self.manual_lamp_type
        try:
            result = lampda_serial.flash_lampda(
                port, lampda, asset=self.latest_release.asset_url
            )
            print("[FLASH RESULT] ", result)
            tk.messagebox.showinfo("Mise à jour", message="Mise à jour réussie")
            self.search_lampda(True)
        except Exception as e:
            tk.messagebox.showinfo(
                "Problème lors de la mise a jour, veuillez la mettre à jour manuellement",
                message=e,
            )

    def ask_user_manual(self, prompt, options):
        """
        fonction permettant d'afficher du texte et
        un choix pour l'utilisateur
        :param prompt:  question
        :param options: choix
        :return:
        """
        # Fenêtre modale pour bloquer le reste
        modal = tk.Toplevel(self)
        modal.title("Sélection du type de lampe")
        modal.geometry("300x150")
        modal.grab_set()  # Rend la fenêtre modale
        modal.transient(self)  # Se comporte comme une fenêtre enfant

        tk.Label(modal, text=prompt, font=("Arial", 12)).pack(pady=10)

        combo = ttk.Combobox(modal, values=options, state="readonly")
        if len(options) > 0:
            combo.current(0)
        combo.pack(pady=5)

        def valider():
            if combo.get():
                self.manual_lamp_type = combo.get()
                modal.destroy()  # Ferme la fenêtre seulement si un choix est fait
            else:
                messagebox.showwarning(
                    "Attention",
                    tx.get_text_translation(tx.WHAT_LAMP_TYPE_SELECT_ID),
                    parent=modal,
                )

        tk.Button(
            modal,
            text="Valider",
            command=valider,
            bg="#4CAF50",
            fg="white",
            font=("Arial", 10, "bold"),
        ).pack(pady=10)

        # Bloque le code ici jusqu'à ce que la fenêtre soit fermée
        self.wait_window(modal)


# -------------------
# Onglet 2 : Chargement et flash d'un fichier UF2
# -------------------
class TabLocalFlash(ttk.Frame):
    """
    permet de mettre a jour une lamp-da depuis un fichier choisi sur l'ordinateur

    """

    def __init__(self, parent):
        """

        :param parent:
        """
        super().__init__(parent)
        self.uf2_path_var = tk.StringVar()
        ttk.Button(
            self, text="Sélectionner un fichier UF2", command=self.browse_uf2_file
        ).pack(pady=10)
        ttk.Button(
            self,
            text="Rechercher les Lamp-da connecté à l'ordinateur",
            command=self.search_lampda,
        ).pack(pady=20)

        ttk.Label(self, textvariable=self.uf2_path_var, wraplength=500).pack(pady=10)

        # Zone où on va afficher les boutons des ports détectés
        self.lampda_container = tk.Frame(self)
        self.lampda_container.pack(fill="both", expand=True, pady=10)

    def browse_uf2_file(self):
        """
        fenetre de dialogue pour trouver un fichier UF2
        :return:
        """
        file_path = filedialog.askopenfilename(
            title="Sélectionnez un fichier UF2", filetypes=[("Fichier UF2", "*.uf2")]
        )
        if file_path:
            self.uf2_path_var.set(file_path)

    def search_lampda(self):
        """
        cherche les lamp-da sur connecté a l'ordinateur
        :return:
        """
        # Nettoyer les anciens boutons
        for widget in self.lampda_container.winfo_children():
            widget.destroy()

        list_lampda = lampda_serial.find_lampda()
        if not list_lampda:
            label = tk.Label(
                self.lampda_container, text=tx.get_text_translation(tx.NO_LAMP_DETECTED_ID), fg="red"
            )
            label.pack()
            return

        # Création d'un bouton pour chaque port détecté
        for port, lampda in list_lampda:
            port_button = ttk.Button(
                self.lampda_container,
                text=tx.get_text_translation(tx.LAMP_UPDATE_ID)%lampda.type,
                command=lambda p=port, t=lampda: self.flash_lampda(p, t),
            )
            port_button.pack(pady=5, fill="x")

    def flash_lampda(self, port, lampda):
        """
        flash la lamp-da si le fichier UF2 existe
        :param port: port de communication avec la lamp-da
        :param lampda: information de la lamp-da
        :return:
        """
        print("on flash ", port, lampda)
        try:
            if os.path.exists(self.uf2_path_var.get()):
                lampda_serial.flash_lampda(
                    port, lampda, asset=[self.uf2_path_var.get()], local=True
                )
                tk.messagebox.showinfo("Status", tx.get_text_translation(tx.LAMP_UPDATE_SUCCESS_ID))
            else:
                tk.messagebox.showinfo("Status", tx.get_text_translation(tx.UF2_FILE_NOT_FOUND_ID))

        except Exception as e:
            tk.messagebox.showinfo(title="Error", message="Error: " + str(e))


# -------------------
# Onglet 3 : Communication série
# -------------------
class TabSerialComm(ttk.Frame):
    """
    onglet permettant de discuter avec les port com de l'ordinateur
    """

    def __init__(self, parent):
        super().__init__(parent)
        self.ser = None

        self.port_combo = ttk.Combobox(self, state="readonly")
        self.port_combo.pack(pady=5)

        self.baud_combo = ttk.Combobox(
            self, values=[9600, 19200, 38400, 57600, 115200], state="disabled"
        )
        self.baud_combo.current(4)
        self.baud_combo.pack(pady=5)

        ttk.Button(self, text="Rafraîchir Ports", command=self.refresh_ports).pack(
            pady=5
        )
        ttk.Button(self, text="Ouvrir Port", command=self.open_serial).pack(pady=5)
        ttk.Button(self, text="Fermer Port", command=self.close_serial).pack(pady=5)

        self.com_output_text = tk.Text(self, height=10)
        self.com_output_text.pack(pady=10, fill="both", expand=True)

        self.com_input_var = tk.StringVar()
        ttk.Entry(self, textvariable=self.com_input_var).pack(
            side=tk.LEFT, fill="x", expand=True, padx=5
        )
        ttk.Button(self, text="Envoyer", command=self.send_serial).pack(
            side=tk.LEFT, padx=5
        )

        self.refresh_ports()

    def refresh_ports(self):
        """
        regarde si il y a des nouveaux port accessible
        :return:
        """
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports:
            self.port_combo.current(0)

    def open_serial(self):
        """
        ouvre une connection série avec le port com demandé
        :return:
        """
        try:
            self.ser = serial.Serial(
                self.port_combo.get(), int(self.baud_combo.get()), timeout=1
            )
            messagebox.showinfo("Succès", f"Connecté à {self.port_combo.get()}")
            threading.Thread(target=self.read_serial, daemon=True).start()

            self.ser.write("h\n".encode())
            self.com_input_var.set("")
        except Exception as e:
            messagebox.showerror("Erreur", f"Impossible d'ouvrir le port : {e}")

    def read_serial(self):
        """
        lis les données sur la liaison série
        :return:
        """
        while self.ser and self.ser.is_open:
            try:
                line = self.ser.readline().decode(errors="ignore").strip()
                if line:
                    self.com_output_text.insert(tk.END, f"{line}\n")
                    self.com_output_text.see(tk.END)
            except serial.SerialException:
                break
            except TypeError:
                break

    def send_serial(self):
        """
        envoie les caractères sur la liaison séri
        :return:
        """
        if self.ser and self.ser.is_open:
            data = self.com_input_var.get() + "\n"
            self.ser.write(data.encode())
            self.com_input_var.set("")

    def close_serial(self, verbose=True):
        """
        ferme le port de communication
        :return:
        """
        if self.ser and self.ser.is_open:
            self.ser.close()
            if verbose:
                messagebox.showinfo("Info", "Port série fermé.")


# -------------------
# Application principale
# -------------------
class FirmwareApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(tx.get_text_translation(tx.WINDOW_TITLE_ID))
        self.geometry("600x600")

        notebook = ttk.Notebook(self)
        notebook.pack(fill="both", expand=True)

        self.debug_com = TabSerialComm(notebook)

        notebook.add(TabOfficialUpdate(notebook), text=tx.get_text_translation(tx.OFFICIAL_MAJ))
        notebook.add(TabLocalFlash(notebook), text=tx.get_text_translation(tx.MANUAL_MAJ))
        notebook.add(self.debug_com, text="Debug")

        notebook.bind("<<NotebookTabChanged>>", self.on_tab_changed)

    def on_tab_changed(self, event):
        """
        Callback déclenché lorsqu'on change d'onglet
        """
        selected_tab = event.widget.select()  # Onglet sélectionné
        tab_text = event.widget.tab(selected_tab, "text")

        # Fermer le port série si on quitte l'onglet "Debug"
        if tab_text != "Debug":
            self.debug_com.close_serial(verbose=False)


if __name__ == "__main__":

    app = FirmwareApp()
    app.mainloop()
