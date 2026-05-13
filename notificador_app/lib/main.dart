import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:http/http.dart' as http;

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'BLE Fall Monitor',
      theme: ThemeData(primarySwatch: Colors.blue, useMaterial3: true),
      home: const HomeScreen(),
    );
  }
}

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  List<ScanResult> scanResults = [];
  BluetoothDevice? connectedDevice;
  StreamSubscription? bleSubscription;
  bool isScanning = false;
  String? savedDeviceId;


//------------ IP SEVIDOR -----------------------
  final String serverUrl = "http://ip:puerto/fall";

  @override
  void initState() {
    super.initState();
    _loadSavedDevice();
  }

  // ---------------- BLE INIT ----------------

  void _loadSavedDevice() async {
    final prefs = await SharedPreferences.getInstance();
    savedDeviceId = prefs.getString("saved_device_id");

    if (savedDeviceId != null) {
      _startAutoConnectScan();
    } else {
      _startManualScan();
    }
  }

  void _startManualScan() {
    if (isScanning) return;
    setState(() => isScanning = true);

    FlutterBluePlus.startScan(timeout: const Duration(seconds: 10));

    FlutterBluePlus.scanResults.listen((results) {
      setState(() => scanResults = results);
    });

    Future.delayed(const Duration(seconds: 10), () {
      setState(() => isScanning = false);
    });
  }

  void _startAutoConnectScan() {
    FlutterBluePlus.startScan(timeout: const Duration(seconds: 20));

    FlutterBluePlus.scanResults.listen((results) {
      for (ScanResult r in results) {
        if (r.device.remoteId.str == savedDeviceId) {
          FlutterBluePlus.stopScan();
          _connectToDevice(r.device);
          break;
        }
      }
    });
  }

  Future<void> _connectToDevice(BluetoothDevice device) async {
    try {
      await device.connect();
      setState(() => connectedDevice = device);

      final prefs = await SharedPreferences.getInstance();
      //con automatica
      await prefs.setString("saved_device_id", device.remoteId.str);

      _discoverServices(device);

      device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          setState(() => connectedDevice = null);
          _startAutoConnectScan();
        }
      });
    } catch (e) {
      print("Error conexión: $e");
    }
  }

  void _discoverServices(BluetoothDevice device) async {
    List<BluetoothService> services = await device.discoverServices();

    for (var service in services) {
      for (var characteristic in service.characteristics) {
        if (characteristic.properties.notify) {
          await characteristic.setNotifyValue(true);

          bleSubscription?.cancel();
          bleSubscription = characteristic.onValueReceived.listen((value) {
            _handleIncomingData(value);
          });
        }
      }
    }
  }

  // ---------------- PARSER DEL MENSAJE DE LA PLACA ----------------

  void _handleIncomingData(List<int> value) async {
    String data = utf8.decode(value).trim();
    print("Recibido BLE: $data");

    try {
      if (!data.startsWith("FALL")) return;

      List<String> parts = data.split("|");

      Map<String, String> map = {};

      for (int i = 1; i < parts.length; i++) {
        var kv = parts[i].split(":");
        if (kv.length == 2) {
          map[kv[0]] = kv[1];
        }
      }

      await _sendFallToServer(
        map["ACC"] ?? "0",
        map["GYR"] ?? "0",
        map["ANG"] ?? "0",
        map["AX"] ?? "0",
        map["AY"] ?? "0",
        map["AZ"] ?? "0",
        "${map["DATE"] ?? ""} ${map["TIME"] ?? ""}",
      );
    } catch (e) {
      print("Error parseando datos: $e");
    }
  }

  // ---------------- ENVÍO AL SERVIDOR ----------------

  Future<void> _sendFallToServer(
    String acc,
    String gyr,
    String ang,
    String ax,
    String ay,
    String az,
    String t,
  ) async {
    try {
      final response = await http.post(
        Uri.parse(serverUrl),
        headers: {"Content-Type": "application/json"},
        body: jsonEncode({
          //unknown por si no manda nombre/ip la placa
          "device_name": connectedDevice?.platformName ?? "unknown",
          "device_id": connectedDevice?.remoteId.str ?? "unknown",

          "acc": double.parse(acc),
          "gyr": double.parse(gyr),
          "ang": double.parse(ang),

          "ax": double.parse(ax),
          "ay": double.parse(ay),
          "az": double.parse(az),

          "timestamp_device": t,
          "timestamp_app": DateTime.now().toIso8601String(),
        }),
      );

      print("Servidor respondió: ${response.statusCode}");
    } catch (e) {
      print("Error enviando al servidor: $e");
    }
  }

  // ---------------- UI ----------------

  void _forgetDevice() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove("saved_device_id");
    await connectedDevice?.disconnect();

    setState(() {
      savedDeviceId = null;
      connectedDevice = null;
      scanResults = [];
    });

    _startManualScan();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Fall Detector BLE"),
        actions: [
          if (savedDeviceId != null)
            IconButton(
              icon: const Icon(Icons.delete),
              onPressed: _forgetDevice,
            )
        ],
      ),
      body: Column(
        children: [
          _statusHeader(),
          Expanded(
            child: connectedDevice != null
                ? _connectedView()
                : _scanListView(),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: _startManualScan,
        child: Icon(isScanning ? Icons.sync : Icons.search),
      ),
    );
  }

  Widget _statusHeader() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(12),
      color: connectedDevice != null
          ? Colors.green.shade100
          : Colors.orange.shade100,
      child: Text(
        connectedDevice != null
            ? "Conectado a ${connectedDevice!.platformName}"
            : "Buscando dispositivo...",
        textAlign: TextAlign.center,
      ),
    );
  }

  Widget _connectedView() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Icon(Icons.bluetooth_connected, size: 80),
          const SizedBox(height: 10),
          Text("Dispositivo: ${connectedDevice!.platformName}"),
          Text("ID: ${connectedDevice!.remoteId.str}"),
          const Padding(
            padding: EdgeInsets.all(16),
            child: Text(
              "Recibiendo datos de caída y enviando al servidor en tiempo real.",
              textAlign: TextAlign.center,
            ),
          )
        ],
      ),
    );
  }

  Widget _scanListView() {
    return ListView.builder(
      itemCount: scanResults.length,
      itemBuilder: (context, index) {
        final device = scanResults[index].device;

        return ListTile(
          leading: const Icon(Icons.developer_board),
          title: Text(device.platformName.isEmpty
              ? "Dispositivo desconocido"
              : device.platformName),
          subtitle: Text(device.remoteId.str),
          onTap: () => _connectToDevice(device),
        );
      },
    );
  }
}