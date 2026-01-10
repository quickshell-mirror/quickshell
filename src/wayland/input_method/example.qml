import QtQuick
import Quickshell
import Quickshell.Wayland
import Quickshell.Io

Item {
    KeyboardTextEdit {
    id: input_method

    transform: function (text: string): string {
      if (input_method.contentHint & ContentHint.LATIN) {
        console.log(input_method.contentHint);
        return text;
      }
      return {
	    "cool": "😎",
		"grinning face" : "😀",
		"grinning face with big eyes" : "😃",
		"grinning face with smiling eyes" : "😄",
		"beaming face with smiling eyes" : "😁",
		"grinning squinting face" : "😆",
		"grinning face with sweat" : "😅",
		"rolling on the floor laughing" : "🤣",
		"face with tears of joy" : "😂",
		"slightly smiling face" : "🙂",
		"upside-down face" : "🙃",
		"melting face" : "🫠",
		"winking face" : "😉",
		"smiling face with smiling eyes" : "😊",
		"smiling face with halo" : "😇"
      }[text];
    }
  }
  IpcHandler {
    target: "emoji"

    function get(): void { input_method.grabKeyboard(); }
  }
}

