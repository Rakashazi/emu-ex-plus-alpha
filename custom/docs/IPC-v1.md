# Snes9x Android Bridge v1

Service-Komponente: `org.snes9x.custom/org.snes9x.bridge.BridgeService`.
Explizit mit `bindService(..., BIND_AUTO_CREATE)` verbinden. Kein Netzwerk.
Binder-Descriptor: `org.snes9x.bridge.v1`; Transaktionscode `IBinder.FIRST_CALL_TRANSACTION` (1).
Request: `writeInterfaceToken`, dann `writeBundle`. Reply: `readException`, dann `readBundle`.
Jeder Request enthält `version` (int 1) und `op` (String). Requests sind synchron (keine One-Way-Transaktionen), nach dem Interface-Token auf 8192 Byte begrenzt und dürfen keine nachfolgenden Parcel-Daten enthalten. Bekannte Felder werden typgeprüft; kleine unbekannte Zusatzfelder bleiben kompatibel. Jede erfolgreiche Antwort enthält `version` (int 1).
Kein automatischer Start einer Activity aus dem Hintergrund. Der Nutzer startet den Emulator und lädt ein Spiel.

## Autorisierung

Service und jede Transaktion erfordern `org.snes9x.bridge.permission.READ`.
Steuerbefehle erfordern zusätzlich `org.snes9x.bridge.permission.CONTROL`.
Beide Berechtigungen sind separat per Android-Dialog anzufordern. Lesende Clients dürfen CONTROL nicht deklarieren.
Autorisierung wird bei jedem Aufruf geprüft. Keine Token-Weitergabe an andere UIDs.

## Lesen

* `info`: `wramSize` = 131072 und `maxRead` = 131072; außerdem `romSize` (int), `loaded`/`ready` (boolean) sowie `generation`/`frame` (long).
* `read`: `region` String `wram` oder `rom` (Standard wram), `offset` int, `length` int 1–131072. Antwort: `bytes` byte[], `generation` long, `frame` long.

WRAM-Offset 0..0x1FFFF entspricht SNES-Bus 0x7E0000..0x7FFFFF. Busadressen müssen Clients selbst umrechnen. Bytes sind unverändert; mehrbyteige SNES-Werte werden in der Regel little endian interpretiert.
ROM-Adressen sind Offsets in die vom Core geladene und normalisierte ROM, kein SNES-Busmapping und kein Kopierheader. Über den Dateiumfang hinausgehende Lesezugriffe scheitern.

WRAM ist ein vollständiger Snapshot nach dem letzten abgeschlossenen Core-Frame. Bei Emulationspause kann derselbe Frame wiederholt gelesen werden. Ein Client pollt z.B. 5–10 Hz im Hintergrundthread und verwirft identische Frame/Generation-Paare. Mehrere Bereiche nur zusammen verwenden, wenn Generation und Frame identisch sind, oder einen einzigen WRAM-Snapshot lesen.

Generation ändert sich beim Spielwechsel, Schließen, Reset und Save-State-Laden. Framezähler zählt ab der aktuellen Generation erfolgreiche Veröffentlichungen. Vor dem ersten Frame und ohne Inhalt ist WRAM nicht verfügbar. Nach Prozessneustart gelten alte Generationswerte nicht mehr; Verbindung neu aufbauen.

## Steuerung (digitales Joypad 0)

* `acquire`: `token` IBinder, vom Client bis `release` am Leben zu halten; exklusiv.
* `queue`: `masks` int[], `frames` int[], gleiche Länge 1–256. Ersetzt die vorige Sequenz. Jeder Schritt hält den gesamten Tastenmaskenzustand für 1–36000 Emulationsframes, Gesamtdauer maximal 36000 Frames. Maske 0 lässt alle Tasten los. Zeit läuft nur mit emulierten Frames, auch bei Fast Forward/Frameskip.
* `pause`: pausiert Sequenz und gibt externe Tasten frei; Restframes bleiben erhalten.
* `resume`: setzt eine pausierte, noch vorhandene Sequenz fort.
* `cancel`: verwirft Sequenz und gibt alle externen Tasten frei.
* `heartbeat`: mindestens einmal pro Sekunde senden. Nach 2 Sekunden ohne Lebenszeichen verfällt die Steuerung. Ablauf wird auch direkt beim nächsten Steuer-/Acquire-Aufruf geprüft, unabhängig von einem verzögerten Watchdog.
* `release`: beendet Steuersitzung, verwirft Sequenz, gibt Tasten frei.

Masken: B=0x8000, Y=0x4000, Select=0x2000, Start=0x1000, Up=0x0800, Down=0x0400, Left=0x0200, Right=0x0100, A=0x0080, X=0x0040, L=0x0020, R=0x0010. Kombinationen bitweise OR. Andere Bits ungültig.

Jede manuelle digitale Controlleränderung verwirft die Automatik. Solange eine manuelle Taste gehalten wird, wird `queue` abgelehnt. Es gibt kein unerwartetes automatisches Wiederaufnehmen. Pause des Emulators, Reset, Save-State-Laden und Spielwechsel verwerfen Sequenzen. Bestehende manuelle Eingaben werden nicht durch `cancel` gelöscht. Maus/Lightgun erhalten keine externen Joypad-Eingaben.

## Fehler und Wiederverbindung

Android transportiert SecurityException (Berechtigung/Sitzung), IllegalArgumentException (VERSION/OPERATION/REGION/RANGE/STEP/SEQUENCE/DURATION), IllegalStateException (NO_CONTENT/MANUAL_INPUT/CONTROL_BUSY) und RemoteException/DeadObjectException (Prozess/Transport). Bei Fehlern keine Nullbytes als gültigen Speicher auswerten. Beim Verbindungsabbruch lokalen Binder verwerfen, erneut binden und Steuerung ausdrücklich neu erwerben. Lesende Clients können automatisch erneut verbinden.

Die API und isolierte Android-Binder-Sicherheitstests sind implementiert; neue Release-Abnahmen mit tatsächlichem Spiel und Signaturmigration bleiben offen. Siehe SECURITY.md und SECURITY-CHECKS.txt. Keine Garantie für zeitliche Fristen aus Wall-Clock-Zeit außer dem Sicherheits-Timeout. Ein Frame entspricht einem Aufruf der bestehenden Core-Schleife; echte Pad-Abtastung und Spielreaktion müssen am Gerät verifiziert werden.
