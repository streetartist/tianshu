import 'package:flutter/material.dart';
import 'core/config/router.dart';
import 'core/theme/app_theme.dart';

/// 应用主组件
class TianshuStudioApp extends StatelessWidget {
  const TianshuStudioApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp.router(
      title: '天枢IoT Studio',
      theme: AppTheme.lightTheme,
      darkTheme: AppTheme.darkTheme,
      routerConfig: appRouter,
      debugShowCheckedModeBanner: false,
    );
  }
}
