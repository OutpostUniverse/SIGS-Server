"""Independent wire-client tests; no use of the C++ encoder or decoder."""
import contextlib
import hashlib
import pathlib
import socket
import struct
import subprocess
import sys
import tempfile
import time
import unittest

EXE = str(pathlib.Path(sys.argv.pop(1)).resolve())

def fields(fmt, *values):
    result = b''
    for token, value in zip(fmt.split(), values, strict=True):
        if token == 's':
            result += value.encode('latin-1') + b'\0'
        elif token == 'b':
            result += struct.pack('>I', len(value)) + value
        else:
            result += struct.pack({'i': '>i', 'l': '>H', 'c': '>B'}[token], value)
    return result

def wire(mid, payload=b''):
    return struct.pack('>IIII', len(payload) + 12, len(payload) + 12, mid, 0x01000000) + payload

class Peer:
    def __init__(self, port):
        self.socket = socket.create_connection(('127.0.0.1', port), timeout=3)
        self.socket.settimeout(3)

    def close(self):
        self.socket.close()

    def send(self, mid, fmt='', *values):
        self.socket.sendall(wire(mid, fields(fmt, *values)))

    def exact(self, size):
        result = b''
        while len(result) < size:
            chunk = self.socket.recv(size - len(result))
            if not chunk:
                raise EOFError('Unexpected EOF')
            result += chunk
        return result

    def recv(self):
        outer, = struct.unpack('>I', self.exact(4))
        assert 12 <= outer <= 1024 * 1024
        inner, mid, flags = struct.unpack('>III', self.exact(12))
        assert inner == outer and flags == 0x01000000
        return mid, self.exact(outer - 12)

    def until(self, mid):
        for _ in range(100):
            current, payload = self.recv()
            if current == mid:
                return payload
        raise AssertionError(f'Message {mid} not found')

class Services(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory()
        cls.path = pathlib.Path(cls.directory.name)
        cls.accounts = cls.path / 'accounts.txt'
        for name in ['Alice', 'Bob', 'Charlie']:
            result = subprocess.run([EXE, '--accounts', str(cls.accounts), '--add-user', name],
                                    input='test-secret\ntest-secret\n', text=True, capture_output=True)
            assert result.returncode == 0, result.stderr
        assert 'test-secret' not in cls.accounts.read_text()
        # Reserve a contiguous test range before starting the server.
        for base in range(28000, 48000, 100):
            try:
                with contextlib.ExitStack() as stack:
                    for port in list(range(base, base + 5)) + list(range(base + 10, base + 13)):
                        sock = stack.enter_context(socket.socket())
                        sock.bind(('127.0.0.1', port))
                    sock = stack.enter_context(socket.socket(socket.AF_INET, socket.SOCK_DGRAM))
                    sock.bind(('127.0.0.1', base + 5))
                cls.base = base
                break
            except OSError:
                continue
        else:
            raise RuntimeError('No test port range available')
        cls.log = open(cls.path / 'server.log', 'w+')
        cls.process = subprocess.Popen([EXE, '--accounts', str(cls.accounts), '--base-port', str(cls.base), '--data', str(cls.path / 'data')],
                                       stdout=cls.log, stderr=cls.log)
        for _ in range(100):
            if cls.process.poll() is not None:
                cls.log.seek(0)
                raise RuntimeError(cls.log.read())
            try:
                with socket.create_connection(('127.0.0.1', cls.base), timeout=.1):
                    return
            except OSError:
                time.sleep(.05)
        raise RuntimeError('Server did not start')

    @classmethod
    def tearDownClass(cls):
        cls.process.terminate()
        cls.process.wait(timeout=5)
        cls.log.close()
        cls.directory.cleanup()

    def peer(self, offset):
        peer = Peer(self.base + offset)
        self.addCleanup(peer.close)
        return peer

    def login(self, name):
        peer = self.peer(1)
        peer.send(601, 's s', name, 'test-secret')
        self.assertEqual(peer.until(311), fields('i s', 0, name))
        peer.until(314)
        peer.until(306)
        return peer

    def enter(self, lobby, name, room_id=1, offset=2):
        lobby.send(602, 'i', room_id)
        self.assertEqual(lobby.until(303), fields('i', 0))
        peer = self.peer(offset)
        record = fields('s s c s s s i', name, name, 0, '', '', '127.0.0.1', 47800)
        peer.socket.sendall(wire(111, record))
        mid, config = peer.recv()
        self.assertEqual(mid, 401)
        self.assertEqual(struct.unpack('>14i', config), (room_id, 2, 4) + (0,) * 10 + (3,))
        self.assertEqual(peer.recv(), (111, record))
        self.assertEqual(peer.recv(), (201, fields('s', name)))
        self.assertEqual(peer.recv(), (402, fields('i i', room_id, 0)))
        peer.until(207)
        peer.send(107)
        return peer

    def transport(self, name, offset=3):
        control = self.peer(offset)
        control.send(102, 's s i', name, '127.0.0.1', 0)
        self.assertEqual(control.until(103), fields('s s i i', '', '127.0.0.1', self.base + offset + 1, self.base + offset + 1))
        data = self.peer(offset + 1)
        data.send(601, 's s', name, '')
        self.assertEqual(data.until(207), b'')
        data.send(107)
        self.assertEqual(control.until(201), fields('s', name))
        control.until(207)
        control.send(107)
        return control, data

    def test_01_validation_fragmentation_and_coalescing(self):
        p = self.peer(0)
        request = wire(616, fields('s s', 'Outpost2_US', '3.0.3.0')) + wire(626, fields('s s', 'OUTPOST2', '1.2.0.5'))
        for cut in [request[:2], request[2:9], request[9:]]:
            p.socket.sendall(cut)
        self.assertEqual(p.recv(), (901, fields('s i', '127.0.0.1', self.base + 1)))
        self.assertEqual(p.recv(), (907, bytes(10)))
        self.assertEqual(p.socket.recv(1), b'')

    def test_02_reject_bad_credentials_and_framing(self):
        p = self.peer(1)
        p.send(601, 's s', 'Alice', 'wrong')
        self.assertEqual(p.until(311), fields('i s', -4, ''))
        p.socket.sendall(bytes.fromhex('00000001'))
        self.assertEqual(p.socket.recv(1), b'')
        q = self.peer(2)
        q.send(111, 's s c s s s i', 'Alice', '', 0, '', '', '127.0.0.1', 47800)
        self.assertEqual(q.socket.recv(1), b'')

    def test_compound_container_and_limits(self):
        p = self.peer(0)
        children = wire(616, fields('s s', 'Outpost2_US', '3.0.3.0'))[4:] + wire(626, fields('s s', 'OUTPOST2', '1.2.0.5'))[4:]
        p.send(1, 'b', children)
        self.assertEqual(p.recv(), (901, fields('s i', '127.0.0.1', self.base + 1)))
        self.assertEqual(p.recv(), (907, bytes(10)))
        for content in [bytes(3), struct.pack('>I', 9999) + bytes(8)]:
            p = self.peer(0)
            p.send(1, 'b', content)
            self.assertEqual(p.socket.recv(1), b'')
        nested = wire(616, fields('s s', 'Outpost2_US', '3.0.3.0'))[4:]
        for _ in range(6):
            nested = wire(1, fields('b', nested))[4:]
        p = self.peer(0)
        p.send(1, 'b', nested)
        self.assertEqual(p.socket.recv(1), b'')
        for offset, mid, fmt, values in [(3, 102, 's s i', ('Alice', '', 0)), (4, 601, 's s', ('Alice', ''))]:
            q = self.peer(offset)
            q.send(mid, fmt, *values)
            self.assertEqual(q.socket.recv(1), b'')
        q = self.peer(0)
        q.send(616, 's s', 'Outpost2_US', '4.0.5.0')
        self.assertEqual(q.socket.recv(1), b'')

    def test_03_multiplayer(self):
        a_login, b_login, c_login = (self.login(n) for n in ['Alice', 'Bob', 'Charlie'])
        alice = self.enter(a_login, 'Alice')
        bob = self.enter(b_login, 'Bob')
        charlie = self.enter(c_login, 'Charlie')
        # Server owns sender identity, irrespective of the supplied sender string.
        bob.send(701, 's s', 'Alice', 'hello')
        self.assertEqual(alice.until(701), fields('s s', 'Bob', 'hello'))
        bob.until(701)
        alice.send(702, 's s s', 'Bob', 'Mallory', 'private')
        self.assertEqual(bob.until(702), fields('s s s', 'Bob', 'Alice', 'private'))
        a_login.send(612, 's', 'bob')
        self.assertEqual(a_login.until(308), fields('i', 1))
        # The UDP probe only echoes after the associated room request.
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
            udp.settimeout(.15)
            datagram = wire(102, fields('s s i', 'Alice', '127.0.0.1', 47800))[4:]
            udp.sendto(datagram, ('127.0.0.1', self.base + 5))
            with self.assertRaises(socket.timeout): udp.recvfrom(1024)
            alice.send(634, 's', 'Alice')
            self.assertEqual(alice.until(110), fields('s i', '127.0.0.1', self.base + 5))
            udp.sendto(datagram, ('127.0.0.1', self.base + 5))
            self.assertEqual(udp.recvfrom(1024)[0], datagram)
        alice.send(605, 's i i i i i i', 'Test', 2, 2, 0, 0, 0, 0)
        self.assertEqual(alice.until(403), fields('s i i i i s i', 'Test', 2, 2, 0, 0, '127.0.0.1', self.base + 3))
        alice.until(404)
        self.assertEqual(alice.until(411), fields('s i i s', 'Test', 0, 0, 'Alice'))
        bob.send(608, 's s i i', 'Test', 'BOGUS', -1, -1)
        self.assertEqual(alice.until(409), fields('s i i s', 'Test', -1, -1, 'Bob'))
        # An unrelated player cannot approve a host's pending request.
        charlie.send(609, 's i', 'Bob', 1)
        alice.send(609, 's i', 'Bob', 1)
        self.assertEqual(bob.until(410), fields('i', 0))
        self.assertEqual(bob.until(411), fields('s i i s', 'Test', 0, 1, 'Bob'))
        bob.until(111)
        ac, ad = self.transport('Alice')
        bc, bd = self.transport('Bob')
        # Later data heartbeats must not repeat the control-side initialization.
        ad.send(107)
        ad.send(703, 'b', b'heartbeat-barrier')
        self.assertEqual(bd.until(704), fields('i i b', 0, 0, b'heartbeat-barrier'))
        ac.socket.settimeout(.15)
        with self.assertRaises(socket.timeout): ac.recv()
        ac.socket.settimeout(3)
        ac.send(619)
        self.assertEqual(alice.until(405), fields('s', 'Test'))
        self.assertEqual(bob.until(405), fields('s', 'Test'))
        blob = b'\x00\xffgame-data'
        ad.send(703, 'b', blob)
        self.assertEqual(bd.until(704), fields('i i b', 0, 0, blob))
        alice.send(615)
        self.assertEqual(bob.until(412), fields('s i i s', 'Test', 0, 0, 'Alice'))
        bob.send(615)
        self.assertEqual(bob.until(406), fields('s', 'Test'))
        # Invitation acceptance can open the transport before 607.
        alice.send(605, 's i i i i i i', 'Invite', 2, 2, 0, 0, 0, 0)
        alice.until(404)
        alice.send(606, 's i i s', 'Invite', 0, 0, 'Charlie')
        self.assertEqual(charlie.until(407), fields('s i i s', 'Invite', 0, 0, 'Alice'))
        self.transport('Charlie')
        charlie.send(607, 's i', 'Alice', 1)
        self.assertEqual(alice.until(408), fields('i s', 1, 'Charlie'))
        self.assertEqual(charlie.until(411), fields('s i i s', 'Invite', 0, 1, 'Charlie'))
        # Invalid messages close only their connection.
        malformed = self.peer(1)
        malformed.socket.sendall(wire(601, b'unterminated'))
        self.assertEqual(malformed.socket.recv(1), b'')
        bob.send(701, 's s', 'Bob', 'still alive')
        self.assertEqual(bob.until(701), fields('s s', 'Bob', 'still alive'))
        # Local room creation keeps distinct room/control/data listeners.
        b_login.send(603, 's s s i c s', 'PrivateRoom', 'A second room', 'OUTPOST2', 8, 1, '')
        self.assertEqual(b_login.until(304), fields('i', 0))
        self.assertEqual(b_login.until(306), fields('s s i i i i c s s s i', 'PrivateRoom', 'OUTPOST2', 2, 0, 0, 0, 0, '', 'A second room', '127.0.0.1', self.base + 10))
        # A full game rejects another admission; independent games may coexist.
        bob.send(608, 's s i i', 'Invite', 'BOGUS', -1, -1)
        self.assertEqual(bob.until(410), fields('i', -1))
        bob.send(605, 's i i i i i i', 'Other', 2, 2, 0, 0, 0, 0)
        self.assertEqual(bob.until(404), fields('i s i', 0, '127.0.0.1', self.base + 3))
        bob.until(411)
        bc, bd = self.transport('Bob')
        # A host alone cannot start a game.
        bc.send(619)
        bob.send(615)
        self.assertEqual(bob.until(406), fields('s', 'Other'))
        # Room disconnect releases ownership; re-entry follows a new admission.
        bob.close()
        self.assertEqual(alice.until(202), fields('s', 'Bob'))
        new_bob = self.enter(b_login, 'Bob', room_id=2, offset=10)
        new_bob.send(605, 's i i i i i i', 'Isolated', 2, 2, 0, 0, 0, 0)
        self.assertEqual(new_bob.until(404), fields('i s i', 0, '127.0.0.1', self.base + 11))
        self.transport('Bob', offset=11)
        # Duplicate account login is rejected even with valid credentials.
        duplicate = self.peer(1)
        duplicate.send(601, 's s', 'alice', 'test-secret')
        self.assertEqual(duplicate.until(311), fields('i s', -4, ''))

class RemainingMessages(unittest.TestCase):
    peer = Services.peer
    login = Services.login
    enter = Services.enter
    transport = Services.transport

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = pathlib.Path(self.directory.name)
        self.accounts = self.path / 'accounts.txt'
        self.data = self.path / 'data'
        for name in ['Alice', 'Bob', 'Charlie']:
            result = subprocess.run([EXE, '--accounts', str(self.accounts), '--add-user', name],
                                    input='test-secret\ntest-secret\n', text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
        self.base = self.free_ports()
        self.process = None
        self.log = open(self.path / 'server.log', 'w+')
        self.addCleanup(self.log.close)
        self.addCleanup(self.stop)

    def free_ports(self):
        for base in range(38000, 48000, 100):
            try:
                with contextlib.ExitStack() as stack:
                    for port in list(range(base, base + 5)) + list(range(base + 10, base + 16)):
                        sock = stack.enter_context(socket.socket())
                        sock.bind(('127.0.0.1', port))
                    sock = stack.enter_context(socket.socket(socket.AF_INET, socket.SOCK_DGRAM))
                    sock.bind(('127.0.0.1', base + 5))
                return base
            except OSError:
                continue
        self.fail('No test ports')

    def stop(self):
        if self.process and self.process.poll() is None:
            self.process.terminate()
            self.process.wait(timeout=5)
        self.process = None

    def start(self, *options):
        self.process = subprocess.Popen([EXE, '--accounts', str(self.accounts), '--data', str(self.data),
                                         '--base-port', str(self.base), *options], stdout=self.log, stderr=self.log)
        for _ in range(100):
            if self.process.poll() is not None:
                self.log.seek(0)
                self.fail(self.log.read())
            try:
                with socket.create_connection(('127.0.0.1', self.base), timeout=.1):
                    return
            except OSError:
                time.sleep(.02)
        self.fail('Server startup timed out')

    def test_room_and_game_names_with_spaces(self):
        self.start()
        alice = self.login('Alice')
        room = self.enter(alice, 'Alice')
        for name in ['', '   ', 'x' * 33, 'bad\nname']:
            room.send(605, 's i i i i i i', name, 2, 2, 0, 0, 0, 0)
            self.assertEqual(room.until(404), fields('i s i', -1, '', 0))
        room.send(605, 's i i i i i i', 'Test best game', 2, 2, 0, 0, 0, 0)
        self.assertEqual(room.until(404), fields('i s i', 0, '127.0.0.1', self.base + 3))
        self.assertEqual(room.until(411), fields('s i i s', 'Test best game', 0, 0, 'Alice'))
        self.transport('Alice')
        bob = self.login('Bob')
        for name in ['', '   ', 'x' * 33, 'bad\nname']:
            bob.send(603, 's s s i c s', name, '', 'OUTPOST2', 4, 1, '')
            self.assertEqual(bob.until(304), fields('i', -1))
        bob.send(603, 's s s i c s', 'Test best room', '', 'OUTPOST2', 4, 1, '')
        self.assertEqual(bob.until(304), fields('i', 0))
        custom_room = self.enter(bob, 'Bob', room_id=2, offset=10)
        custom_room.send(605, 's i i i i i i', 'Test best game', 2, 2, 0, 0, 0, 0)
        self.assertEqual(custom_room.until(404), fields('i s i', 0, '127.0.0.1', self.base + 11))

    def test_profiles_complaints_and_restart(self):
        self.start()
        alice, bob = self.login('Alice'), self.login('Bob')
        room = self.enter(alice, 'Alice')
        other = self.enter(bob, 'Bob')
        alice.send(109, 's s c s s', 'Bob', 'Bob', 255, 'experience', 'biography')
        self.assertEqual(alice.until(313), fields('i', 1))
        expected = fields('s s c s s s i', 'Alice', 'Alice', 63, 'experience', 'biography', '127.0.0.1', 47800)
        self.assertEqual(other.until(111), expected)
        strings = ['Bob', 'Real Name', 'PASSWORD'] + [f'field-{n}' for n in range(3, 14)]
        scalar = [12, 31, -1, 1, 2, 3, 4, 5, 6]
        alice.send(622, ' '.join(['s'] * 14 + ['l', 'l', 'i'] + ['c'] * 6), *strings, *scalar)
        profile = ['Alice', 'Real Name', ''] + strings[3:11]
        expected_member = fields(' '.join(['s'] * 11 + ['l', 'l', 'i'] + ['c'] * 6), *profile, *scalar)
        self.assertEqual(alice.until(314), expected_member)
        self.assertEqual(alice.recv(), (315, fields('i s', 1, 'Alice')))
        bob.send(632, 's', 'Alice')
        expected_public = fields('s s s s l l c c c c c c', 'Alice', strings[5], strings[6][:3], strings[7], 12, 31, *scalar[3:])
        self.assertEqual(bob.until(316), expected_public)
        self.assertNotIn(b'Real Name', expected_public)
        self.assertNotIn(b'PASSWORD', expected_public)
        # Invalid edits return their recovered result instead of disconnecting.
        alice.send(109, 's s c s s', 'Alice', 'Alice', 0, 'x' * 2001, '')
        self.assertEqual(alice.until(313), fields('i', -1))
        invalid_strings = strings.copy(); invalid_strings[1] = 'x' * 257
        alice.send(622, ' '.join(['s'] * 14 + ['l', 'l', 'i'] + ['c'] * 6), *invalid_strings, *scalar)
        self.assertEqual(alice.until(315), fields('i s', -1, 'Alice'))
        alice.send(631, 's s', 'Bob', 'Complaint details')
        alice.send(612, 's', 'Bob')
        alice.until(308)  # complete-packet barrier after the fire-and-forget complaint
        complaints = list((self.data / 'complaints').glob('*.bin'))
        self.assertEqual(len(complaints), 1)
        saved = complaints[0].read_bytes().split(b'\0')
        self.assertEqual(saved[1:4], [b'Alice', b'Bob', b'Complaint details'])
        self.assertNotIn(b'PASSWORD', b''.join(p.read_bytes() for p in (self.data / 'memberships').glob('*.bin')))
        self.stop()
        self.start()
        fresh = self.peer(1)
        fresh.send(601, 's s', 'Alice', 'test-secret')
        self.assertEqual(fresh.until(314), expected_member)
        fresh.until(306)
        fresh.send(602, 'i', 1)
        fresh.until(303)
        reentered = self.peer(2)
        reentered.send(111, 's s c s s s i', 'Alice', 'Alice', 0, '', '', '127.0.0.1', 47800)
        self.assertEqual(reentered.until(111), expected)

    def test_moderation_and_news(self):
        roles = self.path / 'moderators.txt'
        roles.write_text('"Alice"\n')
        news = self.path / 'news.txt'
        news.write_text('7 "Welcome" "A news bulletin"\n')
        self.start('--moderators', str(roles), '--news', str(news))
        alice, bob = self.login('Alice'), self.login('Bob')
        self.assertEqual(bob.until(803), fields('i s s', 7, 'Welcome', 'A news bulletin'))
        room = self.enter(bob, 'Bob')
        bob.send(801, 's', 'forged notice')
        bob.send(802, 's s', 'Alice', 'forged private notice')
        bob.send(804, 's', 'Alice')
        bob.send(612, 's', 'Bob')
        bob.until(308)
        alice.send(802, 's s', 'Bob', 'real private notice')
        self.assertEqual(bob.recv(), (802, fields('s s', 'real private notice', '')))
        alice.send(801, 's', 'real broadcast')
        self.assertEqual(bob.recv(), (801, fields('s', 'real broadcast')))
        # Profile flags cannot grant the server's moderator role.
        bob.send(109, 's s c s s', 'Bob', 'Bob', 255, '', '')
        self.assertEqual(bob.until(313), fields('i', 1))
        bob.send(804, 's', 'Alice')
        alice.send(612, 's', 'Bob')
        self.assertEqual(alice.until(308), fields('i', 1))
        alice.send(804, 's', 'Bob')
        self.assertEqual(bob.socket.recv(1), b'')
        room.until(111)  # profile refresh may have queued before removal
        with self.assertRaises(EOFError): room.recv()

    def test_signup_and_local_password_provisioning(self):
        self.start('--enable-signup')
        signup = self.peer(1)
        values = ['NewMember', 'New Real Name', 'PASSWORD'] + [''] * 9
        signup.send(620, ' '.join(['s'] * 12 + ['l', 'l', 'i'] + ['c'] * 6), *values, *([0] * 9))
        result = signup.until(302)
        self.assertEqual(struct.unpack_from('>i', result)[0], 0)
        self.assertIn(b'no email was sent', result)
        denied = self.peer(1)
        denied.send(601, 's s', 'NewMember', 'PASSWORD')
        self.assertEqual(denied.until(311), fields('i s', -4, ''))
        self.stop()
        result = subprocess.run([EXE, '--accounts', str(self.accounts), '--reset-user', 'NewMember'],
                                input='test-secret\ntest-secret\n', text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.start()
        login = self.login('NewMember')
        self.assertIsNotNone(login)
        self.assertNotIn('PASSWORD', self.accounts.read_text())

    def test_binary32_version_rejection(self):
        self.start('--required-dll-version', '3.5')
        p = self.peer(1)
        p.send(620, ' '.join(['s'] * 12 + ['l', 'l', 'i'] + ['c'] * 6), *(['Member', '', 'PASSWORD'] + [''] * 9), *([0] * 9))
        self.assertEqual(p.recv(), (301, struct.pack('>f', 3.5)))

    def test_legacy_password_room_and_deferred_retirement(self):
        self.start()
        alice, bob, charlie = self.login('Alice'), self.login('Bob'), self.login('Charlie')
        alice.send(603, 's s s i c s', 'Protected', 'Description', 'OUTPOST2', 4, 1, 'local-secret')
        self.assertEqual(alice.until(304), fields('i', 0))
        expected = fields('s s i i i i c s s s i', 'Protected', 'OUTPOST2', 2, 0, 0, 0, 0, 'local-secret', 'Description', '127.0.0.1', self.base + 10)
        self.assertEqual(bob.until(306), expected)
        room = self.enter(alice, 'Alice', 2, 10)
        room.close()
        self.assertEqual(bob.until(309), fields('s', 'Protected'))
        # The departing account must not receive 309 while it considers that room active.
        alice.send(612, 's', 'Alice')
        self.assertEqual(alice.until(308), fields('i', -1))
        # Names remain reserved until the deferred notification can be delivered.
        bob.send(603, 's s s i c s', 'Protected', '', 'OUTPOST2', 4, 1, '')
        self.assertEqual(bob.until(304), fields('i', -1))
        main = self.enter(alice, 'Alice')
        self.assertEqual(alice.until(309), fields('s', 'Protected'))
        bob.send(602, 'i', 2)
        self.assertEqual(bob.until(303), fields('i', -1))
        bob.send(603, 's s s i c s', 'Protected', '', 'OUTPOST2', 4, 1, '')
        self.assertEqual(bob.until(304), fields('i', 0))

    def test_updates_streaming_and_checksum(self):
        package = bytes(range(256)) * 5000  # Larger than the per-client output cap.
        (self.path / 'package.bin').write_bytes(package)
        digest = hashlib.sha256(package).hexdigest()
        manifest = self.path / 'updates.txt'
        manifest.write_text(f'game "OUTPOST2" "old-game" "new-game" 0 "package.bin" {digest}\n'
                            f'sigs "Outpost2_US" "old-sigs" "3.0.3.0" 1 "package.bin" {digest}\n')
        self.start('--updates', str(manifest))
        p = self.peer(0)
        p.send(617)
        self.assertEqual(p.socket.recv(1), b'')
        for kind in ['game', 'sigs']:
            p = self.peer(0)
            p.send(616, 's s', 'Outpost2_US', '3.0.3.0' if kind == 'game' else 'old-sigs')
            if kind == 'game':
                p.until(901)
                p.send(626, 's s', 'OUTPOST2', 'old-game')
                self.assertEqual(p.recv(), (907, fields('i i l', -1, len(package), 0)))
            else:
                self.assertEqual(p.recv(), (903, fields('s i', '3.0.3.0', len(package))))
            p.send(617)
            self.assertEqual(p.recv(), (904, fields('i', len(package))))
            received = bytearray()
            while True:
                mid, payload = p.recv()
                if mid == 906:
                    self.assertEqual(payload, b'')
                    break
                self.assertEqual(mid, 905)
                length, = struct.unpack_from('>I', payload)
                self.assertTrue(0 < length <= 512)
                self.assertEqual(length, len(payload) - 4)
                received += payload[4:]
            self.assertEqual(received, package)
            if kind == 'sigs':
                self.assertEqual(p.recv(), (901, fields('s i', '127.0.0.1', self.base + 1)))
                p.send(626, 's s', 'OUTPOST2', 'current')
                self.assertEqual(p.recv(), (907, bytes(10)))
            self.assertEqual(p.socket.recv(1), b'')
        self.stop()
        manifest.write_text(f'game "OUTPOST2" "old" "new" 0 "package.bin" {"0" * 64}\n')
        result = subprocess.run([EXE, '--updates', str(manifest), '--data', str(self.data)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('checksum', result.stderr)

    def test_capacity_and_canonical_room_identity(self):
        self.start()
        alice, bob, charlie = self.login('Alice'), self.login('Bob'), self.login('Charlie')
        alice.send(603, 's s s i c s', 'Small', '', 'OUTPOST2', 2, 1, '')
        alice.until(304)
        alice.send(602, 'i', 2)
        self.assertEqual(alice.until(303), fields('i', 0))
        first = self.peer(10)
        # String 1 is the username; the server canonicalizes the other identity.
        first.send(111, 's s c s s s i', 'forged-identity', 'Alice', 255, '', '', '127.0.0.1', 47800)
        self.assertEqual(first.until(111), fields('s s c s s s i', 'Alice', 'Alice', 0, '', '', '127.0.0.1', 47800))
        first.until(207)
        self.enter(bob, 'Bob', 2, 10)
        charlie.send(602, 'i', 2)
        self.assertEqual(charlie.until(303), fields('i', -2))
        denied = self.peer(10)
        denied.send(111, 's s c s s s i', 'Charlie', 'Charlie', 0, '', '', '127.0.0.1', 47800)
        self.assertEqual(denied.socket.recv(1), b'')

    def test_sdk_join_permission_parameter(self):
        self.start()
        alice, bob = self.login('Alice'), self.login('Bob')
        a_room, b_room = self.enter(alice, 'Alice'), self.enter(bob, 'Bob')
        a_room.send(605, 's i i i i i i', 'Permission', 2, 2, 0, 0, 0, 0)
        a_room.until(404)
        control, data = self.transport('Alice')
        control.send(627, 's i i', 'Permission', 1, 1)
        # EOF acknowledges consumption of preceding one-way state updates.
        control.socket.shutdown(socket.SHUT_WR)
        self.assertEqual(control.socket.recv(1), b'')
        b_room.send(608, 's s i i', 'Permission', 'BOGUS', -1, -1)
        self.assertEqual(b_room.until(410), fields('i', -1))
        control = self.peer(3)
        control.send(102, 's s i', 'Alice', '127.0.0.1', 0)
        control.until(103)
        control.send(627, 's i i', 'Permission', 1, 2)
        control.socket.shutdown(socket.SHUT_WR)
        self.assertEqual(control.socket.recv(1), b'')
        b_room.send(608, 's s i i', 'Permission', 'BOGUS', -1, -1)
        self.assertEqual(b_room.until(410), fields('i', 0))
        self.assertEqual(b_room.until(411), fields('s i i s', 'Permission', 0, 1, 'Bob'))

    def test_registry_coverage_and_unresolved_diagnostics(self):
        result = subprocess.run([EXE, '--message-coverage'], capture_output=True, text=True, check=True)
        rows = [line.split('\t') for line in result.stdout.splitlines()[1:]]
        self.assertEqual(len(rows), 118)
        self.assertEqual(len({r[0] for r in rows}), 118)
        self.assertEqual(next(r[1] for r in rows if r[0] == '621'), 'unresolved')
        self.start()
        p = self.login('Alice')
        p.send(610, 's i s', 'redacted-sensitive', 0, 'do-not-log')
        p.send(621, 's s', 'redacted-password', 'redacted-secret')
        p.send(612, 's', 'Alice')
        self.assertEqual(p.until(308), fields('i', -1))
        self.stop()
        log = (self.path / 'server.log').read_text()
        self.assertIn('id=610 status=unresolved', log)
        self.assertIn('id=621 status=unresolved', log)
        self.assertNotIn('redacted-', log)
        self.assertNotIn('do-not-log', log)

if __name__ == '__main__':
    unittest.main()
