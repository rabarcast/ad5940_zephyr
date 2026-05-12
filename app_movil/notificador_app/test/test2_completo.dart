import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:notificador_app/main.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  group('HomeScreen Tests', () {

    testWidgets('La app carga correctamente', (WidgetTester tester) async {
      await tester.pumpWidget(
        MaterialApp(
          home: HomeScreen(),
        ),
      );

      // Verifica que aparece el título
      expect(find.text('Seleccionar Dispositivo'), findsOneWidget);
    });

    testWidgets('Botón de simulación funciona', (WidgetTester tester) async {
      await tester.pumpWidget(
        MaterialApp(
          home: HomeScreen(),
        ),
      );

      // Busca el botón flotante (simulación FALL)
      final fab = find.byType(FloatingActionButton);

      expect(fab, findsOneWidget);

      // Pulsa el botón
      await tester.tap(fab);
      await tester.pump();

      // No podemos verificar la notificación directamente,
      // pero comprobamos que no rompe la app
      expect(find.byType(HomeScreen), findsOneWidget);
    });

    testWidgets('Renderiza lista o estado conectado', (WidgetTester tester) async {
      await tester.pumpWidget(
        MaterialApp(
          home: HomeScreen(),
        ),
      );

      await tester.pump();

      // Puede haber lista o texto de conexión
      final listFinder = find.byType(ListView);
      final textFinder = find.textContaining("Conectado");

      expect(
        listFinder.evaluate().isNotEmpty || textFinder.evaluate().isNotEmpty,
        true,
      );
    });

  });
}