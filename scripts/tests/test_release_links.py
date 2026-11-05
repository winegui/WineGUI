import json
import os
from pathlib import Path
import subprocess
import threading
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

SCRIPT = Path(__file__).resolve().parents[1] / 'create-release-links.sh'


class ReleaseLinksTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def reply(self, status, body):
                self.send_response(status)
                self.end_headers()
                self.wfile.write(body.encode())

            def do_GET(self):
                state = cls.state
                state['gets'] += 1
                if state.get('get_status'):
                    return self.reply(state['get_status'], '{"message":"failure"}')
                if state.get('invalid'):
                    return self.reply(200, state['invalid'])
                page = int(parse_qs(urlparse(self.path).query)['page'][0])
                self.reply(200, json.dumps(state['links'][(page-1)*100:page*100]))

            def do_POST(self):
                state = cls.state
                state['posts'] += 1
                fields = parse_qs(self.rfile.read(int(self.headers['Content-Length'])).decode())
                if state.get('fail_at') == state['posts']:
                    return self.reply(500, '{"message":"failed creation"}')
                link = {key: values[0] for key, values in fields.items()}
                state['links'].append(link)
                self.reply(201, state.get('post_body', json.dumps(link)))

        cls.server = HTTPServer(('127.0.0.1', 0), Handler)
        cls.thread = threading.Thread(target=cls.server.serve_forever, kwargs={'poll_interval': 0.01})
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown()
        cls.thread.join()
        cls.server.server_close()

    def setUp(self):
        type(self).state = {'links': [], 'posts': 0, 'gets': 0}

    def run_script(self):
        return subprocess.run(['bash', str(SCRIPT)], env=dict(os.environ, APP_VERSION='v4.5.2', CI_JOB_TOKEN='dummy',
                              CI_PROJECT_ID='66', CI_API_V4_URL=f'http://127.0.0.1:{self.server.server_port}'),
                              capture_output=True, text=True, timeout=10)

    def test_create_and_repeat(self):
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(len(self.state['links']), 9)
        self.assertTrue(self.state['links'][-1]['url'].endswith('-x86_64.AppImage'))
        self.assertEqual(self.run_script().returncode, 0)
        self.assertEqual(self.state['posts'], 9)

    def test_partial_failure_then_resume(self):
        self.state['fail_at'] = 4
        self.assertNotEqual(self.run_script().returncode, 0)
        self.assertEqual(self.state['posts'], 4)
        self.assertEqual(len(self.state['links']), 3)
        self.assertEqual(self.run_script().returncode, 0)
        self.assertEqual(len(self.state['links']), 9)
        self.assertEqual(self.state['posts'], 10)

    def test_get_errors_do_not_create_links(self):
        for status in (401, 404, 500):
            with self.subTest(status=status):
                self.state['get_status'] = status
                self.assertNotEqual(self.run_script().returncode, 0)
                self.assertEqual(self.state['posts'], 0)

    def test_invalid_responses_do_not_create_links(self):
        for body in ('not json', '{}', '[{}]'):
            with self.subTest(body=body):
                self.state['invalid'] = body
                self.assertNotEqual(self.run_script().returncode, 0)
                self.assertEqual(self.state['posts'], 0)

    def test_pagination_keeps_existing_links(self):
        self.assertEqual(self.run_script().returncode, 0)
        unrelated = [{'name': f'Other {i}', 'url': f'https://example.org/{i}', 'link_type': 'other'} for i in range(100)]
        self.state['links'] = unrelated + self.state['links']
        self.state['gets'] = 0
        self.assertEqual(self.run_script().returncode, 0)
        self.assertEqual(self.state['gets'], 2)
        self.assertEqual(self.state['posts'], 9)

    def test_conflicting_link_fails(self):
        self.assertEqual(self.run_script().returncode, 0)
        self.state['links'][0]['url'] = 'https://example.org/wrong'
        result = self.run_script()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Conflicting release link', result.stderr)
        self.assertEqual(self.state['posts'], 9)

    def test_invalid_creation_response_stops_immediately(self):
        self.state['post_body'] = '{}'
        self.assertNotEqual(self.run_script().returncode, 0)
        self.assertEqual(self.state['posts'], 1)
