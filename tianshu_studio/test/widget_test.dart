import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:tianshu_studio/app.dart';

void main() {
  testWidgets('App loads correctly', (WidgetTester tester) async {
    await tester.pumpWidget(
      const ProviderScope(child: TianshuStudioApp()),
    );
    expect(find.text('天枢IoT Studio'), findsOneWidget);
  });
}
