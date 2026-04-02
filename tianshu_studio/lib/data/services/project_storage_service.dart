import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../../data/models/models.dart';

/// 项目存储服务
class ProjectStorageService {
  static const _projectsKey = 'tianshu_projects';

  /// 获取所有项目
  static Future<List<Project>> loadProjects() async {
    try {
      final prefs = await SharedPreferences.getInstance();
      final jsonStr = prefs.getString(_projectsKey);
      if (jsonStr == null) return [];

      final List<dynamic> jsonList = jsonDecode(jsonStr);
      return jsonList.map((json) => Project.fromJson(json)).toList();
    } catch (e) {
      debugPrint('加载项目失败: $e');
      return [];
    }
  }

  /// 保存项目
  static Future<bool> saveProject(Project project) async {
    try {
      final projects = await loadProjects();
      final index = projects.indexWhere((p) => p.id == project.id);

      if (index >= 0) {
        projects[index] = project;
      } else {
        projects.add(project);
      }

      return _saveProjects(projects);
    } catch (e) {
      debugPrint('保存项目失败: $e');
      return false;
    }
  }

  /// 删除项目
  static Future<bool> deleteProject(String projectId) async {
    try {
      final projects = await loadProjects();
      projects.removeWhere((p) => p.id == projectId);
      return _saveProjects(projects);
    } catch (e) {
      debugPrint('删除项目失败: $e');
      return false;
    }
  }

  static Future<bool> _saveProjects(List<Project> projects) async {
    final prefs = await SharedPreferences.getInstance();
    final jsonStr = jsonEncode(projects.map((p) => p.toJson()).toList());
    return prefs.setString(_projectsKey, jsonStr);
  }
}
