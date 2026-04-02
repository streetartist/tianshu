import '../../../data/models/models.dart';
import '../models/generated_project.dart';
import '../templates/iot_service_template.dart';

/// 服务层生成器
class ServiceGenerator {
  static List<GeneratedFile> generate(
    Project project, {
    Map<String, Map<String, String>>? deviceBindings,
  }) {
    return [
      _generateIoTService(deviceBindings),
      _generateApiService(project),
    ];
  }

  static GeneratedFile _generateIoTService(Map<String, Map<String, String>>? bindings) {
    return GeneratedFile(
      path: 'lib/services/iot_service.dart',
      content: IoTServiceTemplate.generate(defaultBindings: bindings),
    );
  }

  static GeneratedFile _generateApiService(Project project) {
    final serverUrl = project.settings.serverUrl;
    return GeneratedFile(
      path: 'lib/services/api_service.dart',
      content: '''import 'package:dio/dio.dart';

class ApiService {
  static final ApiService _instance = ApiService._();
  static ApiService get instance => _instance;

  late Dio _dio;
  String? _token;

  ApiService._() {
    _dio = Dio(BaseOptions(
      baseUrl: '$serverUrl',
      connectTimeout: const Duration(seconds: 30),
    ));
  }

  void setToken(String? token) {
    _token = token;
  }

  Future<Response> get(String path) async {
    return _dio.get(path);
  }

  Future<Response> post(String path, {dynamic data}) async {
    return _dio.post(path, data: data);
  }
}
''',
    );
  }
}
