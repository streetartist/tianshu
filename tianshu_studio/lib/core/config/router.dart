import 'package:go_router/go_router.dart';
import '../../features/canvas/canvas_page.dart';
import '../../features/project/project_list_page.dart';
import '../../features/settings/settings_page.dart';

/// 应用路由配置
final appRouter = GoRouter(
  initialLocation: '/',
  routes: [
    GoRoute(
      path: '/',
      builder: (context, state) => const ProjectListPage(),
    ),
    GoRoute(
      path: '/canvas/:projectId',
      builder: (context, state) {
        final projectId = state.pathParameters['projectId']!;
        return CanvasEditorPage(projectId: projectId);
      },
    ),
    GoRoute(
      path: '/settings',
      builder: (context, state) => const SettingsPage(),
    ),
  ],
);
