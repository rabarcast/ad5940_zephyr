import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:geolocator/geolocator.dart';

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
      title: 'Fall Detector BLE',
      theme: ThemeData(primarySwatch: Colors.red, useMaterial3: true),
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

  final FlutterLocalNotificationsPlugin notificationsPlugin = FlutterLocalNotificationsPlugin();

  @override
  void initState() {
    super.initState();
    _initNotifications();
    _checkLocationPermissions();
    _loadSavedDevice();
  }

  // --- CONFIGURACIÓN INICIAL ---

  void _initNotifications() async {
    const androidSettings = AndroidInitializationSettings('@mipmap/ic_launcher');
    await notificationsPlugin.initialize(const InitializationSettings(android: androidSettings));
  }

  void _checkLocationPermissions() async {
    LocationPermission permission = await Geolocator.checkPermission();
    if (permission == LocationPermission.denied) {
      await Geolocator.requestPermission();
    }
  }

  void _loadSavedDevice() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      savedDeviceId = prefs.getString("saved_device_id");
    });

    if (savedDeviceId != null) {
      _startAutoConnectScan();
    } else {
      _startManualScan();
    }
  }

  // --- LÓGICA BLE ---

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

  void _startAutoConnectScan() async {
    print("Buscando dispositivo guardado: $savedDeviceId");
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
      
      // Guardar ID para siempre
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString("saved_device_id", device.remoteId.str);

      _discoverServices(device);

      device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          setState(() => connectedDevice = null);
          _startAutoConnectScan(); // Reintento automático
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
        // Importante: Debes conocer el UUID de tu placa o buscar una que permita notify/write
        if (characteristic.properties.notify) {
          await characteristic.setNotifyValue(true);
          bleSubscription?.cancel();
          bleSubscription = characteristic.onValueReceived.listen((value) {
            _handleIncomingData(value, characteristic);
          });
        }
      }
    }
  }

  // --- LÓGICA DE EVENTOS ---

  void _handleIncomingData(List<int> value, BluetoothCharacteristic char) async {
    String data = utf8.decode(value).trim().toUpperCase();
    print("Recibido: $data");

    if (data.contains("FALL")) {
      _showNotification();
      _sendFallResponse(char);
    }
  }

  Future<void> _sendFallResponse(BluetoothCharacteristic char) async {
    try {
      Position pos = await Geolocator.getCurrentPosition();
      String timestamp = DateTime.now().toIso8601String();
      String response = "LAT:${pos.latitude},LON:${pos.longitude},T:$timestamp";
      
      await char.write(utf8.encode(response));
      print("Respuesta enviada a placa");
    } catch (e) {
      print("Error al responder: $e");
    }
  }

  void _showNotification() async {
    const details = NotificationDetails(
      android: AndroidNotificationDetails(
        'fall_id', 'Alertas de Caída',
        importance: Importance.max,
        priority: Priority.high,
        icon: '@mipmap/ic_launcher',
      ),
    );
    await notificationsPlugin.show(1, "🚨 CAÍDA DETECTADA", "Se está enviando tu ubicación...", details);
  }

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

  // --- INTERFAZ ---

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Detector de Caídas"),
        actions: [
          if (savedDeviceId != null)
            IconButton(icon: const Icon(Icons.delete_forever), onPressed: _forgetDevice)
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
    bool isConnected = connectedDevice != null;
    return Container(
      color: isConnected ? Colors.green.shade100 : Colors.orange.shade100,
      width: double.infinity,
      padding: const EdgeInsets.all(12),
      child: Text(
        isConnected ? "✅ Conectado a la placa" : "⚠️ Buscando placa...",
        textAlign: TextAlign.center,
        style: const TextStyle(fontWeight: FontWeight.bold),
      ),
    );
  }

  Widget _connectedView() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Icon(Icons.bluetooth_connected, size: 80, color: Colors.blue),
          const SizedBox(height: 20),
          Text("ID: ${connectedDevice!.remoteId.str}"),
          const Padding(
            padding: EdgeInsets.all(20),
            child: Text("La app está escuchando en segundo plano. Si la placa envía 'FALL', se activará el protocolo.",
              textAlign: TextAlign.center),
          ),
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
          title: Text(device.platformName.isEmpty ? "Placa Desconocida" : device.platformName),
          subtitle: Text(device.remoteId.str),
          onTap: () => _connectToDevice(device),
        );
      },
    );
  }
}