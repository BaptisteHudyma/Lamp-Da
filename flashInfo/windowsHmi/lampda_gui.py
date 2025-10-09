import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import requests
import serial
import serial.tools.list_ports
import threading
import lampda_serial
import os
import time

# used to translate text strings
import TextLangage as tx

storedState = {}
def enable_hierachy(panel, enable:bool):
    panel.active = enable

    if(panel.config().get("state")):
        widgetName = id(panel)
        if not enable:
            if widgetName:
                storedState[widgetName] = panel["state"]
            panel["state"] = "disabled"
        elif widgetName != None and widgetName in storedState:
            panel["state"] = storedState[widgetName]

    for widget in panel.winfo_children():
        enable_hierachy(widget, enable)

# -------------------
# Onglet 1 : Mise à jour depuis GitHub
# -------------------
class TabOfficialUpdate(ttk.Frame):
    """
    permet de mettre a jour la lamp-da depuis le depot officiel sur git
    si la version de la lampe est trop vielle, il faudra choisir manuellement le type de la lampe
    """

    def __init__(self, parent, main_window, lamp_releases):
        """
        :param parent: frame parent pour placer l'onglet
        """
        super().__init__(parent)
        self.main_window = main_window
        ttk.Button(
            self,
            text=tx.get_text_translation(tx.FIND_CONNECTED_DRIVE_ID),
            command=self.search_lampda,
        ).pack(pady=20)

        # Zone où on va afficher les boutons des ports détectés
        self.lampda_container = tk.Frame(self)
        self.lampda_container.pack(fill="both", expand=True, pady=10)

        self.all_release = lamp_releases
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

        if self.list_lampda is None or only_update is False:
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
        self.buttonList = []
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
            self.buttonList.append(port_button)

            if etat != tk.NORMAL:
                port_force_button = ttk.Button(
                    self.lampda_container,
                    text=tx.get_text_translation(tx.FORCE_UPDATE_ID),
                    command=lambda p=port, t=lampda: self.flash_lampda(p, t),
                    state=tk.NORMAL,
                )
                port_force_button.pack(padx=5, fill="x")

    def flash_lampda(self, port, lampda):
        #unspecified lamp type
        shouldSkipReset = False
        if lampda.type not in ["simple", "indexable"]:
            self.ask_user_manual(
                tx.get_text_translation(tx.WHAT_LAMP_TYPE_ID), ["simple", "indexable"]
            )
            if self.manual_lamp_type == None:
                return

            lampda.type = self.manual_lamp_type
            shouldSkipReset = True

        # start a work thread
        self.flash_thread = threading.Thread(target=lambda p=port, t=lampda, b=shouldSkipReset: self.flash_lampda_action(p, t, b), args=())

        self.load_bar = ttk.Progressbar(self)
        self.load_bar.pack(padx = 10, pady = (0,10))
        # the load_bar needs to be configured for indeterminate amount of bouncing
        self.load_bar.config(mode='indeterminate', maximum=100, value=0, length = None)
        # here is for speed of bounce
        self.load_bar.start(8)

        enable_hierachy(self.main_window, False)
        # wait for flash
        self.flash_thread.start()

    def flash_lampda_action(self, port, lampda: lampda_serial.LampDa, skip_reset):
        """

        :param port: port de la lamp-da a mettre a jour
        :param lampda: information de la lamp-da a mettre a jour

        """
        print("on flash ", port, lampda)
        try:
            lampda_serial.flash_lampda(
                port, lampda, asset=self.latest_release.asset_url, skip_reset=skip_reset
            )

            tk.messagebox.showinfo("Info", message=tx.get_text_translation(tx.LAMP_UPDATE_SUCCESS_ID))
            self.search_lampda(True)

            self.load_bar.stop()
            self.load_bar.destroy()
            enable_hierachy(self.main_window, True)
            self.main_window.update_connected_devices()

        except Exception as e:
            tk.messagebox.showinfo(
                tx.get_text_translation(tx.MAJ_FAILED),
                message=e,
            )
            self.load_bar.stop()
            self.load_bar.destroy()
            enable_hierachy(self.main_window, True)
            self.main_window.update_connected_devices()

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
            text=tx.get_text_translation(tx.SELECT_UF2),
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

    def __init__(self, parent, main_window):
        """

        :param parent:
        """
        super().__init__(parent)

        self.main_window = main_window
        self.uf2_path_var = tk.StringVar()
        ttk.Button(
            self, text=tx.get_text_translation(tx.SELECT_UF2), command=self.browse_uf2_file
        ).pack(pady=10)
        ttk.Button(
            self,
            text=tx.get_text_translation(tx.FIND_CONNECTED_DRIVE_ID),
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
            title=tx.get_text_translation(tx.SELECT_UF2), filetypes=[("Fichier UF2", "*.uf2")]
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
        self.buttonList = []
        for port, lampda in list_lampda:
            port_button = ttk.Button(
                self.lampda_container,
                text=tx.get_text_translation(tx.LAMP_UPDATE_ID)%lampda.type,
                command=lambda p=port, t=lampda: self.flash_lampda(p, t),
            )
            port_button.pack(pady=5, fill="x")

            self.buttonList.append(port_button)

    def flash_lampda(self, port, lampda):
        shouldSkipReset = False
        if lampda.type not in ["simple", "indexable"]:
            shouldSkipReset = True
        # start a work thread
        flash_thread = threading.Thread(target=lambda p=port, t=lampda, b=shouldSkipReset: self.flash_lampda_action(p, t, b), args=())

        self.load_bar = ttk.Progressbar(self)
        self.load_bar.pack(padx = 10, pady = (0,10))
        # the load_bar needs to be configured for indeterminate amount of bouncing
        self.load_bar.config(mode='indeterminate', maximum=100, value=0, length = None)
        # here is for speed of bounce
        self.load_bar.start(8)

        enable_hierachy(self.main_window, False)
        # wait for flash
        flash_thread.start()

    def flash_lampda_action(self, port, lampda: lampda_serial.LampDa, skip_reset):
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
                    port, lampda, asset=[self.uf2_path_var.get()], local=True, skip_reset=skip_reset
                )
                tk.messagebox.showinfo("Status", tx.get_text_translation(tx.LAMP_UPDATE_SUCCESS_ID))
            else:
                tk.messagebox.showinfo("Status", tx.get_text_translation(tx.UF2_FILE_NOT_FOUND_ID))

            self.load_bar.stop()
            self.load_bar.destroy()
            enable_hierachy(self.main_window, True)
            self.main_window.update_connected_devices()

        except Exception as e:
            tk.messagebox.showinfo(title="Error", message="Error: " + str(e))

            self.load_bar.stop()
            self.load_bar.destroy()
            enable_hierachy(self.main_window, True)
            self.main_window.update_connected_devices()


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

        ttk.Button(self, text=tx.get_text_translation(tx.RERESH_PORTS_ID), command=self.refresh_ports).pack(
            pady=5
        )
        ttk.Button(self, text=tx.get_text_translation(tx.OPEN_PORT_ID), command=self.open_serial).pack(pady=5)

        self.com_output_text = tk.Text(self, height=10)
        self.com_output_text.pack(pady=10, fill="both", expand=True)
        self.com_output_text.config(state=tk.DISABLED)

        self.com_input_var = tk.StringVar()
        ttk.Entry(self, textvariable=self.com_input_var).pack(
            side=tk.LEFT, fill="x", expand=True, padx=5
        )
        ttk.Button(self, text=tx.get_text_translation(tx.SEND), command=self.send_serial).pack(
            side=tk.LEFT, padx=5
        )

        self.refresh_ports()

    def refresh_ports(self):
        """
        regarde si il y a des nouveaux port accessible
        :return:
        """
        self.close_serial()
        self.baud_combo.state = "disabled"

        ports = [port.device for port in serial.tools.list_ports.comports() if self.is_port_a_lampda(port.device)]
        if ports:
            self.port_combo["values"] = ports
            self.port_combo.current(0)
        else:
            self.port_combo.set("")

    def is_port_a_lampda(self, port):
        """Send a test command to detect if the serial port is a lampda system"""
        try:
            with serial.Serial(port, int(self.baud_combo.get()), timeout=0.3) as serialP :
                serialP.write("h\n".encode())
                line = serialP.readline().decode(errors="ignore").strip()
                if line:
                    return True
        except:
            pass
        return False

    def open_serial(self):
        """
        ouvre une connection série avec le port com demandé
        :return:
        """
        try:
            self.ser = serial.Serial(
                self.port_combo.get(), int(self.baud_combo.get()), timeout=1
            )
            threading.Thread(target=self.read_serial, daemon=True).start()

            self.ser.write("h\n".encode())
            self.com_input_var.set("")

            print("opened a serial connection")
        except Exception as e:
            self.refresh_ports()
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
                    self.com_output_text.config(state=tk.NORMAL)
                    self.com_output_text.insert(tk.END, f"{line}\n")
                    self.com_output_text.see(tk.END)
                    self.com_output_text.config(state=tk.DISABLED)
            except serial.SerialException:
                break
            except TypeError:
                break

    def send_serial(self):
        """
        envoie les caractères sur la liaison série
        :return:
        """
        try:
            if self.ser and self.ser.is_open:
                data = self.com_input_var.get() + "\n"
                self.ser.write(data.encode())
                self.com_input_var.set("")
        except:
            self.refresh_ports()

    def close_serial(self):
        """
        ferme le port de communication
        :return:
        """
        if self.ser and self.ser.is_open:
            self.ser.close()

        self.com_output_text.config(state=tk.NORMAL)
        self.com_output_text.delete('1.0', tk.END)
        self.com_output_text.config(state=tk.DISABLED)

class TabParameters(ttk.Frame):
    """
    onglet permettant de regler les parametres
    """

    def __init__(self, notebook: ttk.Notebook, main_windows, lamp_releases):
        super().__init__(notebook)

        self.notebook = notebook
        self.main_windows = main_windows
        self.langage_button = ttk.Button(
                self,
                text=tx.get_text_translation(tx.SWITCH_LANGAGE_ID),
                command=self.change_langage,
            ).pack(pady=20)

        self.maj_display_text = tk.Text(self, height=10)
        self.maj_display_text.pack(pady=10, fill="both", expand=True)
        self.maj_display_text.config(state=tk.NORMAL)
        for release in lamp_releases:
            # display each release changes
            self.maj_display_text.insert(tk.END, f"{release.tag}:\n\n{release.creation_date}\n\n")
            self.maj_display_text.insert(tk.END, f"{release.description}\n")
            self.maj_display_text.insert(tk.END, f"\n\n")
            self.maj_display_text.insert(tk.END, f"---------------------\n")
        if len(lamp_releases) <= 0:
            self.maj_display_text.insert(tk.END, tx.get_text_translation(tx.GET_RELEASES_FAILED_ID))

        self.maj_display_text.config(state=tk.DISABLED)

    def change_langage(self):
        tx.handle.change_langage()
        self.notebook.destroy()
        self.main_windows.init_notebook()



# -------------------
# Application principale
# -------------------
class FirmwareApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(tx.get_text_translation(tx.WINDOW_TITLE_ID))
        self.geometry("850x600")

        # CALL ONCE to avoid behing rejected
        try :
            self.lamp_releases = lampda_serial.get_releases()
        except Exception as e:
            print("Get realse exception:", e)
            pass

        # if no release, respond with the error
        if len(self.lamp_releases) <= 0:
            tk.messagebox.showinfo(
                "Alert",
                message=tx.get_text_translation(tx.GET_RELEASES_FAILED_ID),
            )
            self.lamp_releases = []

        self.init_notebook()

    def init_notebook(self):
        self.notebook = ttk.Notebook(self)
        self.notebook.pack(fill="both", expand=True)

        self.debug_com = TabSerialComm(self.notebook)
        self.local_flash = TabLocalFlash(self.notebook, self)
        self.parameters = TabParameters(self.notebook, self, self.lamp_releases)
        if len(self.lamp_releases) > 0:
            self.official_updates = TabOfficialUpdate(self.notebook, self, self.lamp_releases)
            self.notebook.add(self.official_updates, text=tx.get_text_translation(tx.OFFICIAL_MAJ))

        self.notebook.add(self.local_flash, text=tx.get_text_translation(tx.MANUAL_MAJ))
        self.notebook.add(self.debug_com, text="Debug")
        self.notebook.add(self.parameters, text="Params")

        self.notebook.bind("<<NotebookTabChanged>>", self.on_tab_changed)

    def update_connected_devices(self):
        if self.official_updates:
            self.official_updates.search_lampda(only_update=True)
        if self.local_flash:
            self.local_flash.search_lampda()
        if self.debug_com:
            self.debug_com.refresh_ports()

    def on_tab_changed(self, event):
        """
        Callback déclenché lorsqu'on change d'onglet
        """
        selected_tab = event.widget.select()  # Onglet sélectionné
        tab_text = event.widget.tab(selected_tab, "text")

        # Fermer le port série si on quitte l'onglet "Debug"
        if tab_text != "Debug":
            self.debug_com.close_serial()

if __name__ == "__main__":

    app = FirmwareApp()
    app.mainloop()
