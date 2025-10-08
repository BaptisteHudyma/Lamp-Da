# Store the different string text displayed in the app, to allow for a langage change

WINDOW_TITLE_ID = "windows_title"
FIND_CONNECTED_DRIVE_ID = "find_serial"
OFFICIAL_MAJ = "maj_off"
MANUAL_MAJ = "maj_man"
LAMP_ALREADY_UPDATED_ID = "lampUpToDate"
LAMP_UPDATE_ID = "lampUpdate"
LAMP_UPDATE_TARGET_ID = "lampUpdateTarget"
NO_LAMP_DETECTED_ID = "noConnected"
FORCE_UPDATE_ID = "forceUpdate"
LAMP_NO_PROGRAM_ID = "noProg"
USB_DRIVE_NOT_FOUND_ID = "driveNotFound"
LAMP_UPDATE_SUCCESS_ID = "updateSuccess"
UF2_FILE_NOT_FOUND_ID = "uf2NotFound"
WHAT_LAMP_TYPE_ID = "whatLampType"
WHAT_LAMP_TYPE_SELECT_ID = "whatlampTypeSelect"

lang = "Francais"
# lang = "English"

def get_text_translation(text_id):
    try:
        if lang == "Francais":
            return French[text_id]
        if lang == "English":
            return English[text_id]
    except:
        pass
    return "Err:no_token_for_language"

French = {
    WINDOW_TITLE_ID: "FlashApp: Mise à jours de votre Lamp-da",
    FIND_CONNECTED_DRIVE_ID: "Rechercher les Lamp-da connectées",
    OFFICIAL_MAJ: "Mise à jour officiel",
    MANUAL_MAJ: "Mise à jour manuelle",
    LAMP_ALREADY_UPDATED_ID : "La lampe \'%s\' est déjà à jour",
    LAMP_UPDATE_ID : "Mettre à jour la lampe \'%s\'",
    LAMP_UPDATE_TARGET_ID : "Mettre à jour la lampe \'%s\' :  %s -> %s",
    NO_LAMP_DETECTED_ID : "Aucune lamp-da détectée",
    FORCE_UPDATE_ID: "Forcer la mise a jour",
    LAMP_NO_PROGRAM_ID: "Lampe sans programme: %s",
    USB_DRIVE_NOT_FOUND_ID: "Lecteur USB non trouvé",
    LAMP_UPDATE_SUCCESS_ID: "La lampe a bien été mise a jour",
    UF2_FILE_NOT_FOUND_ID: "Le fichier UF2 n'existe pas",
    WHAT_LAMP_TYPE_ID: "Indiquez le type de lampe :",
    WHAT_LAMP_TYPE_SELECT_ID: "Veuillez sélectionner un type de lampe avant de continuer."
}

English = {
    WINDOW_TITLE_ID: "FlashApp: Update your Lamp-da",
    FIND_CONNECTED_DRIVE_ID: "Search for connected Lamp-da",
    OFFICIAL_MAJ: "Official update",
    MANUAL_MAJ: "Manual update",
    LAMP_ALREADY_UPDATED_ID : "The lamp \'%s\' is already up to date",
    LAMP_UPDATE_ID : "Update the lamp \'%s\'",
    LAMP_UPDATE_TARGET_ID : "Update the lamp \'%s\' :  %s -> %s",
    NO_LAMP_DETECTED_ID : "No lamp detected",
    FORCE_UPDATE_ID: "Force the update",
    LAMP_NO_PROGRAM_ID: "Lamp without a software %s",
    USB_DRIVE_NOT_FOUND_ID: "USB drive not found",
    LAMP_UPDATE_SUCCESS_ID: "The lamp was successfully updated",
    UF2_FILE_NOT_FOUND_ID: "The UF2 file do not exist",
    WHAT_LAMP_TYPE_ID: "Specify the lamp type :",
    WHAT_LAMP_TYPE_SELECT_ID: "Please select a lamp type before proceeding."
}
