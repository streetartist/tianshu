import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import 'package:uuid/uuid.dart';
import '../../data/models/models.dart';
import '../../data/services/project_storage_service.dart';
import '../../shared/providers/app_providers.dart';

/// 项目列表Provider
final projectListProvider = FutureProvider<List<Project>>((ref) async {
  return ProjectStorageService.loadProjects();
});

/// 项目列表页面
class ProjectListPage extends ConsumerWidget {
  const ProjectListPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final projectsAsync = ref.watch(projectListProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('天枢IoT Studio'),
        actions: [
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: () => context.push('/settings'),
          ),
        ],
      ),
      body: projectsAsync.when(
        data: (projects) => _buildProjectList(context, ref, projects),
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (e, _) => Center(child: Text('加载失败: $e')),
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: () => _showCreateDialog(context, ref),
        icon: const Icon(Icons.add),
        label: const Text('新建项目'),
      ),
    );
  }

  Widget _buildProjectList(BuildContext context, WidgetRef ref, List<Project> projects) {
    if (projects.isEmpty) {
      return const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(Icons.folder_open, size: 64, color: Colors.grey),
            SizedBox(height: 16),
            Text('暂无项目，点击下方按钮创建'),
          ],
        ),
      );
    }

    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: projects.length,
      itemBuilder: (context, index) {
        final project = projects[index];
        return _buildProjectCard(context, ref, project);
      },
    );
  }

  Widget _buildProjectCard(BuildContext context, WidgetRef ref, Project project) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: ListTile(
        leading: const Icon(Icons.phone_android, size: 40),
        title: Text(project.name),
        subtitle: Text('${project.pages.length} 个页面'),
        trailing: PopupMenuButton(
          itemBuilder: (context) => [
            const PopupMenuItem(value: 'delete', child: Text('删除')),
          ],
          onSelected: (value) {
            if (value == 'delete') _deleteProject(context, ref, project.id);
          },
        ),
        onTap: () => _openProject(context, ref, project),
      ),
    );
  }

  void _openProject(BuildContext context, WidgetRef ref, Project project) {
    ref.read(currentProjectProvider.notifier).state = project;
    context.push('/canvas/${project.id}');
  }

  Future<void> _deleteProject(BuildContext context, WidgetRef ref, String id) async {
    await ProjectStorageService.deleteProject(id);
    ref.invalidate(projectListProvider);
  }

  void _showCreateDialog(BuildContext context, WidgetRef ref) {
    final controller = TextEditingController(text: '新项目');
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('新建项目'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(labelText: '项目名称'),
          autofocus: true,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('取消'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              _createProject(context, ref, controller.text);
            },
            child: const Text('创建'),
          ),
        ],
      ),
    );
  }

  void _createProject(BuildContext context, WidgetRef ref, String name) {
    final project = Project(
      id: const Uuid().v4(),
      name: name.isEmpty ? '新项目' : name,
    );
    ref.read(currentProjectProvider.notifier).state = project;
    ref.invalidate(projectListProvider);
    context.push('/canvas/${project.id}');
  }
}
