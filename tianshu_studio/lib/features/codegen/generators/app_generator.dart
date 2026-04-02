import '../../../data/models/models.dart';
import '../models/generated_project.dart';

/// app.dart生成器
class AppGenerator {
  static GeneratedFile generate(Project project) {
    return GeneratedFile(
      path: 'lib/app.dart',
      content: '''import 'package:flutter/material.dart';
import 'pages/home_page.dart';

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: '${project.name}',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: const HomePage(),
      debugShowCheckedModeBanner: false,
    );
  }
}
''',
    );
  }
}
