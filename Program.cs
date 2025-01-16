using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Runtime.Versioning;
using System.Collections.Generic;
using System.Drawing;

[SupportedOSPlatform("windows")]
public class MainForm : Form
{
    private TableLayoutPanel table;
    private Dictionary<IntPtr, TableRow> deviceRows = new();
    private const int WM_INPUT = 0x00FF;
    private const uint RIM_TYPEKEYBOARD = 1;
    private NotifyIcon trayIcon;
    private ContextMenuStrip trayMenu;

    public class TableRow
    {
        public Label Name { get; set; }
        public TextBox DeviceId { get; set; }
        public Label Keycode { get; set; }

        // For handling the flash effect
        private Color originalColor;
        private Timer flashTimer;

        public TableRow()
        {
            Name = new Label() { Dock = DockStyle.Fill, AutoSize = true, TextAlign = ContentAlignment.MiddleCenter };
            DeviceId = new TextBox() { Dock = DockStyle.Fill, AutoSize = true, TextAlign = HorizontalAlignment.Center, ReadOnly = true, BorderStyle = BorderStyle.None };
            Keycode = new Label() { Dock = DockStyle.Fill, AutoSize = true, TextAlign = ContentAlignment.MiddleCenter };

            // Setup flash timer
            flashTimer = new Timer();
            flashTimer.Interval = 100; // 100ms flash duration
            flashTimer.Tick += (s, e) =>
            {
                Keycode.BackColor = originalColor;
                flashTimer.Stop();
            };

            originalColor = Keycode.BackColor;
        }

        public void Flash()
        {
            Keycode.BackColor = Color.LightBlue;
            flashTimer.Stop(); // Reset any ongoing flash
            flashTimer.Start();
        }
    }

    public MainForm()
    {
        Text = "HIDeous";
        Size = new Size(800, 300);

        // Create main table layout
        table = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 3,
            RowCount = 2, // Header row + empty row
            CellBorderStyle = TableLayoutPanelCellBorderStyle.Single
        };

        // Configure columns
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 40));  // Name
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 30));  // Device ID
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 30));  // Keycode

        // Add headers
        AddHeader("Name", 0);
        AddHeader("Device ID", 1);
        AddHeader("Keycode", 2);

        Controls.Add(table);

        // Register for raw input
        RegisterRawInput();

        // Find and list keyboards
        ListKeyboards();

        // Initialize tray icon and menu
        trayMenu = new ContextMenuStrip();
        trayMenu.Items.Add("Restore", null, OnRestore);
        trayMenu.Items.Add("Exit", null, OnExit);

        trayIcon = new NotifyIcon
        {
            Text = "HIDEous",
            Icon = new Icon("Resources/appicon.ico"),
            ContextMenuStrip = trayMenu,
            Visible = true
        };

        trayIcon.DoubleClick += (s, e) => OnRestore(s, e);

        // Minimize to tray by default
        ShowInTaskbar = false;
    }

    private void OnRestore(object? sender, EventArgs e)
    {
        Show();
        WindowState = FormWindowState.Normal;
        ShowInTaskbar = true;
    }

    private void OnExit(object? sender, EventArgs e)
    {
        trayIcon.Visible = false;
        Application.Exit();
    }

    protected override void OnResize(EventArgs e)
    {
        base.OnResize(e);
        if (WindowState == FormWindowState.Minimized)
        {
            Hide();
            ShowInTaskbar = false;
        }
    }

    private void RegisterRawInput()
    {
        var device = new RAWINPUTDEVICE
        {
            usUsagePage = 0x01,
            usUsage = 0x06,
            dwFlags = 0x00000100, // RIDEV_INPUTSINK
            hwndTarget = Handle
        };

        if (!RegisterRawInputDevices(new RAWINPUTDEVICE[] { device }, 1, Marshal.SizeOf(typeof(RAWINPUTDEVICE))))
        {
            MessageBox.Show("Failed to register raw input device");
        }
    }

    private string? GetDeviceName(IntPtr device)
    {
        try
        {
            uint size = 0;
            uint result = GetRawInputDeviceInfo(device, 0x20000007, IntPtr.Zero, ref size);

            if (result == uint.MaxValue)
            {
                int errorCode = Marshal.GetLastWin32Error();
                string errorMessage = new System.ComponentModel.Win32Exception(errorCode).Message;
                MessageBox.Show($"Initial GetRawInputDeviceInfo call failed.\nError code: {errorCode}\nMessage: {errorMessage}");
                return null;
            }

            IntPtr namePtr = Marshal.AllocHGlobal((int)size * 2);
            try
            {
                result = GetRawInputDeviceInfo(device, 0x20000007, namePtr, ref size);

                if (result == uint.MaxValue)
                {
                    int errorCode = Marshal.GetLastWin32Error();
                    string errorMessage = new System.ComponentModel.Win32Exception(errorCode).Message;
                    MessageBox.Show($"GetRawInputDeviceInfo call failed.\nError code: {errorCode}\nMessage: {errorMessage}");
                    return null;
                }

                string devicePath = Marshal.PtrToStringAuto(namePtr) ?? string.Empty;

                // Convert the raw device path into a more readable format
                if (devicePath.Contains("VID_") && devicePath.Contains("PID_"))
                {
                    int vidIndex = devicePath.IndexOf("VID_");
                    int pidIndex = devicePath.IndexOf("PID_");

                    if (vidIndex != -1 && pidIndex != -1)
                    {
                        string vid = devicePath.Substring(vidIndex, 8); // VID_XXXX
                        string pid = devicePath.Substring(pidIndex, 8); // PID_XXXX
                        return $"Keyboard ({vid} {pid})";
                    }
                }

                return devicePath;
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Exception occurred while getting device name: {ex.Message}");
                return null;
            }
            finally
            {
                Marshal.FreeHGlobal(namePtr);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Exception occurred in GetDeviceName: {ex.Message}");
            return null;
        }
    }

    private void AddHeader(string text, int column)
    {
        var header = new Label
        {
            Text = text,
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleCenter,
            Font = new Font(Font, FontStyle.Bold),
            BackColor = Color.LightGray
        };
        table.Controls.Add(header, column, 0);
    }

    private void ListKeyboards()
    {
        uint deviceCount = 0;
        GetRawInputDeviceList(IntPtr.Zero, ref deviceCount, (uint)Marshal.SizeOf(typeof(RAWINPUTDEVICELIST)));

        if (deviceCount == 0) return;

        var deviceList = Marshal.AllocHGlobal((int)(deviceCount * Marshal.SizeOf(typeof(RAWINPUTDEVICELIST))));
        try
        {
            GetRawInputDeviceList(deviceList, ref deviceCount, (uint)Marshal.SizeOf(typeof(RAWINPUTDEVICELIST)));

            for (int i = 0; i < deviceCount; i++)
            {
                var device = Marshal.PtrToStructure<RAWINPUTDEVICELIST>(
                    IntPtr.Add(deviceList, i * Marshal.SizeOf(typeof(RAWINPUTDEVICELIST))));

                if (device.dwType == RIM_TYPEKEYBOARD)
                {
                    var name = GetDeviceName(device.hDevice);
                    AddKeyboardRow(device.hDevice, name ?? "Unknown Keyboard");
                }
            }
        }
        catch
        {
            MessageBox.Show("Failed to list keyboards");
        }
        finally
        {
            AddEmptyRow();
            Marshal.FreeHGlobal(deviceList);
        }
    }

    private void AddKeyboardRow(IntPtr deviceHandle, string deviceName)
    {
        var row = new TableRow();

        // Create a TextBox for the DeviceId that is read-only
        var deviceIdTextBox = new TextBox();
        deviceIdTextBox.Text = deviceHandle.ToString();
        deviceIdTextBox.ReadOnly = true;
        deviceIdTextBox.BorderStyle = BorderStyle.None; // Optional: to make it look like a label
        deviceIdTextBox.TextAlign = HorizontalAlignment.Center;
        deviceIdTextBox.Dock = DockStyle.Fill;


        row.DeviceId = deviceIdTextBox;
        row.Name.Text = deviceName;
        row.Keycode.Text = "No key pressed";

        // Add new row to table
        table.RowCount++;
        int rowIndex = table.RowCount - 1;

        // Add controls to table
        table.Controls.Add(row.Name, 0, rowIndex);
        table.Controls.Add(row.DeviceId, 1, rowIndex);
        table.Controls.Add(row.Keycode, 2, rowIndex);

        deviceRows[deviceHandle] = row;
    }

    private void AddEmptyRow()
    {
        table.RowCount++;
        int rowIndex = table.RowCount - 1;

        for (int i = 0; i < table.ColumnCount; i++)
        {
            var label = new Label { Dock = DockStyle.Fill, TextAlign = ContentAlignment.MiddleCenter };
            table.Controls.Add(label, i, rowIndex);
        }
    }

    protected override void WndProc(ref Message message)
    {
        if (message.Msg == WM_INPUT)
        {
            uint size = 0;
            GetRawInputData(message.LParam, 0x10000003, IntPtr.Zero, ref size, (uint)Marshal.SizeOf(typeof(RAWINPUTHEADER)));

            var buffer = Marshal.AllocHGlobal((int)size);
            try
            {
                if (GetRawInputData(message.LParam, 0x10000003, buffer, ref size, (uint)Marshal.SizeOf(typeof(RAWINPUTHEADER))) == size)
                {
                    var input = Marshal.PtrToStructure<RAWINPUT>(buffer);
                    if (input.header.dwType == RIM_TYPEKEYBOARD)
                    {
                        if (deviceRows.TryGetValue(input.header.hDevice, out var row))
                        {
                            row.Keycode.Text = $"VK: {input.keyboard.VKey:X2}";
                            row.Flash();
                        }
                    }
                }
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }
        base.WndProc(ref message);
    }

    // Required structures and imports
    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUTDEVICELIST
    {
        public IntPtr hDevice;
        public uint dwType;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUTDEVICE
    {
        public ushort usUsagePage;
        public ushort usUsage;
        public uint dwFlags;
        public IntPtr hwndTarget;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUTHEADER
    {
        public uint dwType;
        public uint dwSize;
        public IntPtr hDevice;
        public IntPtr wParam;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWKEYBOARD
    {
        public ushort MakeCode;
        public ushort Flags;
        public ushort Reserved;
        public ushort VKey;
        public uint Message;
        public uint ExtraInformation;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct RAWINPUT
    {
        public RAWINPUTHEADER header;
        public RAWKEYBOARD keyboard;
    }

    [DllImport("user32.dll")]
    private static extern bool RegisterRawInputDevices(RAWINPUTDEVICE[] pRawInputDevices, uint uiNumDevices, int cbSize);

    [DllImport("user32.dll")]
    private static extern uint GetRawInputDeviceList(IntPtr pRawInputDeviceList, ref uint puiNumDevices, uint cbSize);

    [DllImport("User32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    private static extern uint GetRawInputDeviceInfo(IntPtr hDevice, uint uiCommand, IntPtr pData, ref uint pcbSize);

    [DllImport("user32.dll")]
    private static extern uint GetRawInputData(IntPtr hRawInput, uint uiCommand, IntPtr pData, ref uint pcbSize, uint cbSize);
}

public class Program
{
    [STAThread]
    static void Main()
    {
        try
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Error starting application: {ex.Message}\n\n{ex.StackTrace}");
        }
    }
}
