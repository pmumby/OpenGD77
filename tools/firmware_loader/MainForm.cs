/*
 * Copyright (C)2019 Roger Clark. VK3KYY
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.ComponentModel.Design;
using System.IO;
using System.Net;
using System.Text.RegularExpressions;
#if (LINUX_BUILD)
#else
using UsbLibrary;
#endif

namespace GD77_FirmwareLoader
{
	public class WebClientAsync : WebClient
	{
		private int timeoutMS;
		private System.Timers.Timer timer;

		public WebClientAsync(int timeoutSeconds)
		{
			timeoutMS = timeoutSeconds * 1000;

			timer = new System.Timers.Timer(timeoutMS);
			System.Timers.ElapsedEventHandler handler = null;

			handler = ((sender, args) =>
			{
				this.CancelAsync();
				timer.Stop();
				timer.Elapsed -= handler;
			});

			timer.Elapsed += handler;
			timer.Enabled = true;
		}

		protected override WebRequest GetWebRequest(Uri address)
		{
			WebRequest request = base.GetWebRequest(address);
			request.Timeout = timeoutMS;
			((HttpWebRequest)request).ReadWriteTimeout = timeoutMS;

			return request;
		}

		protected override void OnDownloadProgressChanged(DownloadProgressChangedEventArgs e)
		{
			base.OnDownloadProgressChanged(e);
			timer.Stop();
			timer.Start();
		}
	}

	public partial class MainForm : Form
	{
		private static String tempFile = "";
		private WebClientAsync wc = null;

		private void PostActivated(object sender, EventArgs e)
		{
			this.Activated -= PostActivated;

			this.Activate();
		}

		public MainForm()
		{
			InitializeComponent();
			this.Activated += PostActivated;
		}

		private void rbModel_CheckedChanged(object sender, EventArgs e)
		{
			RadioButton rb = sender as RadioButton;

			if (rb != null)
			{
				if (rb.Checked)
				{
					FirmwareLoader.outputType = (FirmwareLoader.OutputType)rb.Tag;
				}
				btnDownload.Enabled = true;
				btnOpenFile.Enabled = true;
			}
		}

		private void enableUI(bool state)
		{
			this.grpboxModel.Enabled = state;
			this.btnDetect.Enabled = state;
			this.btnDownload.Enabled = state;
			this.btnOpenFile.Enabled = state;
		}

		private void downloadProgressChangedCallback(object sender, DownloadProgressChangedEventArgs ev)
		{
			this.progressBar.Value = ev.ProgressPercentage;
		}

		private DialogResult DialogBox(String title, String message, String btn1Label = "&Yes", String btn2Label = "&No", String btn3Label = "&Cancel")
		{
			int buttonX = 10;
			int buttonY = 120 - 25 - 5;
			Form form = new System.Windows.Forms.Form();
			Label label = new System.Windows.Forms.Label();
			Button button1 = new System.Windows.Forms.Button();
			Button button2 = new System.Windows.Forms.Button();
			Button button3 = new System.Windows.Forms.Button();

			form.SuspendLayout();

			if (btn1Label.Length <= 0)
			{
				button1.Visible = false;
				button1.Enabled = false;
			}

			if (btn2Label.Length <= 0)
			{
				button2.Visible = false;
				button2.Enabled = false;
			}

			if (btn1Label.Length <= 0 || btn2Label.Length <= 0)
			{
				buttonX += 120 + 10;
			}

			form.Text = title;

			// Label
			label.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			label.Location = new System.Drawing.Point(13, 13);
			label.Name = "LblMessage";
			label.Size = new System.Drawing.Size(380 - (13 * 2), (120 - 24 - 13 - 13));
			label.Text = message;
			label.TextAlign = ContentAlignment.MiddleCenter;

			// Button 1
			button1.Text = btn1Label ?? string.Empty;
			button1.Name = "btnYes";
			button1.Location = new System.Drawing.Point(buttonX, buttonY);
			button1.Size = new System.Drawing.Size(120, 24);
			button1.UseVisualStyleBackColor = true;

			if (button1.Visible)
			{
				buttonX += 120 + 10;
			}

			// Button 2
			button2.Text = btn2Label ?? string.Empty;
			button2.Name = "btnNo";
			button2.Location = new System.Drawing.Point(buttonX, buttonY);
			button2.Size = new System.Drawing.Size(120, 24);
			button2.UseVisualStyleBackColor = true;

			// Button 3
			button3.Text = btn3Label ?? string.Empty;
			button3.Location = new System.Drawing.Point((380 - 100 - 10), buttonY);
			button3.Name = "btnCancel";
			button3.Size = new System.Drawing.Size(100, 24);
			button3.UseVisualStyleBackColor = true;

			// Assign results
			button1.DialogResult = DialogResult.Yes;
			button2.DialogResult = DialogResult.No;
			button3.DialogResult = DialogResult.Cancel;

			form.ClientSize = new System.Drawing.Size(396, 107);
			form.Controls.Add(label);

			if (button1.Visible)
			{
				form.Controls.Add(button1);
			}

			if (button2.Visible)
			{
				form.Controls.Add(button2);
			}

			form.Controls.Add(button3);

			form.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			form.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			form.ClientSize = new System.Drawing.Size(380, 120);
			form.KeyPreview = true;
			form.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			form.MinimizeBox = false;
			form.MaximizeBox = false;
			form.AcceptButton = (button1.Visible == false ? button2 : button1);
			form.CancelButton = button3;
			form.ResumeLayout(false);

			DialogResult dialogResult = form.ShowDialog();

			return dialogResult;
		}

		private void downloadedGetReleaseAndDevelURLs(String[] lines, ref String releaseURL, ref String develURL)
		{
			String patternFormat = "";
			String pattern;

			releaseURL = "";
			develURL = "";

			// Define Regex's patterm, according to current Model selection
			switch (FirmwareLoader.outputType)
			{
				case FirmwareLoader.OutputType.OutputType_GD77:
					patternFormat = @"/rogerclarkmelbourne/OpenGD77/releases/download/{0}([0-9\.]+)/OpenGD77\.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_GD77S:
					patternFormat = @"/rogerclarkmelbourne/OpenGD77/releases/download/{0}([0-9\.]+)/OpenGD77S\.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_DM1801:
					patternFormat = @"/rogerclarkmelbourne/OpenGD77/releases/download/{0}([0-9\.]+)/OpenDM1801\.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_RD5R:
					patternFormat = @"/rogerclarkmelbourne/OpenGD77/releases/download/{0}([0-9\.]+)/OpenRD5R\.sgl";
					break;
			}

			pattern = String.Format(patternFormat, 'R');
			foreach (String l in lines)
			{
				Match match = Regex.Match(l, pattern, RegexOptions.IgnoreCase);

				if (match.Success)
				{
					releaseURL = match.Groups[0].Value;
					break;
				}
			}

			pattern = String.Format(patternFormat, 'D');
			foreach (String l in lines)
			{
				Match match = Regex.Match(l, pattern, RegexOptions.IgnoreCase);

				if (match.Success)
				{
					develURL = match.Groups[0].Value;
					break;
				}
			}
		}

		private void downloadStringCompletedCallback(object sender, DownloadStringCompletedEventArgs ev)
		{
			if (ev.Cancelled)
			{
				MessageBox.Show("Download has been canceled.", "Timeout", MessageBoxButtons.OK, MessageBoxIcon.Error);

				enableUI(true);
				this.progressBar.Visible = false;
				return;
			}
			else if (ev.Error != null)
			{
				MessageBox.Show(ev.Error.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

				enableUI(true);
				this.progressBar.Visible = false;
				return;
			}

			String result = ev.Result;
			String urlBase = "http://github.com";
			String urlFW = "";
			String patternR = "", patternD = "";
			String releaseURL = "", develURL = "";

			this.progressBar.Visible = false;

			// Looking for firmware's URL
			String[] lines = result.Split('\n');

			downloadedGetReleaseAndDevelURLs(lines, ref releaseURL, ref develURL);

			// Is firmware's URL found ?
			if ((releaseURL.Length > 0) || (develURL.Length > 0))
			{
				String message;
				String[] buttonsLabel = new String[2];

				buttonsLabel[0] = "&Stable ";
				buttonsLabel[1] = "&Unstable ";

				// Extract release version
				patternR = @"/R([0-9\.]+)/";
				patternD = @"/D([0-9\.]+)/";

				Match matchR = Regex.Match(releaseURL, patternR, RegexOptions.IgnoreCase);
				Match matchD = Regex.Match(develURL, patternD, RegexOptions.IgnoreCase);

				if (matchR.Success && (releaseURL.Length > 0))
				{
					buttonsLabel[0] += matchR.Groups[0].Value.Trim('/').Remove(0, 1);
				}
				else
				{
					buttonsLabel[0] = "";
				}

				if (matchD.Success && (develURL.Length > 0))
				{
					buttonsLabel[1] += matchD.Groups[0].Value.Trim('/').Remove(0, 1);
				}
				else
				{
					buttonsLabel[1] = "";
				}

				if ((releaseURL.Length > 0) && (develURL.Length > 0))
				{
					message = "It will download and install a firmware.\n\nPlease choose between Stable and Development version.";
				}
				else
				{
					message = "It will download and install a firmware.\n\nPlease make you choice.";
				}

				DialogResult res = DialogBox("Question", message, buttonsLabel[0], buttonsLabel[1]);

				switch (res)
				{
					case DialogResult.Yes:
						// Stable
						Console.WriteLine("STABLE");
						urlFW = releaseURL;
						break;

					case DialogResult.No:
						// Devel
						Console.WriteLine("UNSTABLE");
						urlFW = develURL;
						break;

					case DialogResult.Cancel:
						enableUI(true);
						return;
				}

				tempFile = System.IO.Path.GetTempPath() + Guid.NewGuid().ToString() + ".sgl";

				// Download the firmware binary to a temporary file
				try
				{
					Application.DoEvents();
					this.progressBar.Value = 0;
					this.progressBar.Visible = true;
					wc.DownloadFileAsync(new Uri(urlBase + urlFW), tempFile);
				}
				catch (Exception ex)
				{
					MessageBox.Show("Error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

					if (File.Exists(tempFile))
						File.Delete(tempFile);

					enableUI(true);
					this.progressBar.Visible = false;
					return;
				}
			}
			else
			{
				MessageBox.Show(String.Format("Error: unable to find a firmware for your {0} transceiver.", FirmwareLoader.getModelName()), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				enableUI(true);
			}
		}

		private void downloadFileCompletedCallback(object sender, AsyncCompletedEventArgs ev)
		{
			this.progressBar.Visible = false;
			this.progressBar.Value = 0;

			if (ev.Cancelled)
			{
				MessageBox.Show("Download has been canceled.", "Timeout", MessageBoxButtons.OK, MessageBoxIcon.Error);

				enableUI(true);
				return;
			}
			else if (ev.Error != null)
			{
				MessageBox.Show(ev.Error.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

				enableUI(true);
				return;
			}

			// Now flash the downloaded firmware
			try
			{
				FrmProgress frmProgress = new FrmProgress();
				frmProgress.SetLabel("");
				frmProgress.SetProgressPercentage(0);
				frmProgress.FormBorderStyle = FormBorderStyle.FixedSingle;
				frmProgress.MaximizeBox = false;
				frmProgress.Show();

				if (FirmwareLoader.UploadFirmware(tempFile, frmProgress) != 0)
				{
					MessageBox.Show("Error: Unable to upload the firmware to the " + FirmwareLoader.getModelName(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				}

				frmProgress.Close();
			}
			catch (Exception ex)
			{
				MessageBox.Show("Error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}

			// Cleanup
			if (File.Exists(tempFile))
				File.Delete(tempFile);

			enableUI(true);
		}

		private void btnDetect_Click(object sender, EventArgs e)
		{
			this.btnDetect.Enabled = false;

			FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_UNKNOWN;// FirmwareLoader.probeModel();

			if ((FirmwareLoader.outputType < FirmwareLoader.OutputType.OutputType_GD77) || (FirmwareLoader.outputType > FirmwareLoader.OutputType.OutputType_RD5R))
			{
				MessageBox.Show("Error: Unable to detect your radio.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_GD77;
			}

			this.rbModels[(int)FirmwareLoader.outputType].Checked = true;
			this.btnDetect.Enabled = true;
		}

		private void btnDownload_Click(object sender, EventArgs e)
		{
			Uri uri = new Uri("https://github.com/rogerclarkmelbourne/OpenGD77/releases");

			wc = new WebClientAsync(40);
			ServicePointManager.SecurityProtocol = SecurityProtocolType.Ssl3 | SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;

			this.progressBar.Value = 0;

			wc.DownloadProgressChanged += new DownloadProgressChangedEventHandler(downloadProgressChangedCallback);
			wc.DownloadStringCompleted += new DownloadStringCompletedEventHandler(downloadStringCompletedCallback);
			wc.DownloadFileCompleted += new AsyncCompletedEventHandler(downloadFileCompletedCallback);

			this.progressBar.Visible = true;
			enableUI(false);

			// Retrieve release webpage
			try
			{
				Application.DoEvents();
				wc.DownloadStringAsync(uri);
			}
			catch (Exception ex)
			{
				MessageBox.Show("Error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				enableUI(true);
				this.progressBar.Visible = false;
				return;
			}
		}

		private void btnOpenFile_Click(object sender, EventArgs e)
		{
			OpenFileDialog openFileDialog1 = new OpenFileDialog();
			openFileDialog1.Filter = "firmware files (*.sgl)|*.sgl|binary files (*.bin)|*.bin|All files (*.*)|*.*";
			openFileDialog1.RestoreDirectory = true;

			if (openFileDialog1.ShowDialog() == DialogResult.OK)
			{
				try
				{
					enableUI(false);
					FrmProgress frmProgress = new FrmProgress();
					frmProgress.SetLabel("");
					frmProgress.SetProgressPercentage(0);
					frmProgress.FormBorderStyle = FormBorderStyle.FixedSingle;
					frmProgress.MaximizeBox = false;
					frmProgress.Show();
					FirmwareLoader.UploadFirmware(openFileDialog1.FileName, frmProgress);
					frmProgress.Close();
				}
				catch (Exception)
				{

				}
				enableUI(true);
			}
		}
	}
}
